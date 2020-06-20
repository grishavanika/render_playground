#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <vector>
#include <filesystem>
#include <algorithm>

#include <cstdlib>
#include <cassert>
#include <cstdint>

#include "model.h"

namespace fs = std::filesystem;

static void Panic(bool condition)
{
    if (!condition)
    {
        assert(false);
        std::exit(1);
    }
}

struct AssimpTexture
{
    unsigned int material_index;
    std::string path;
};

struct AssimpMesh
{
    std::vector<Vertex> vertices;
    std::vector<Index> indices;
    AssimpTexture texture_diffuse;
};

struct AssimpModel
{
    struct Blob
    {
        unsigned int material_index;
        unsigned char* data; // stbi_image_free(data)
        unsigned int width;
        unsigned int height;
    };
    std::vector<AssimpMesh> meshes;
    std::vector<Blob> materials;
};

// Model's loading with Assimp comes from learnopengl.com:
// https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/model.h
static AssimpMesh Assimp_ProcessMesh(const aiScene& scene, const aiMesh& mesh)
{
    static_assert(std::is_same_v<float, ai_real>);

    Panic(mesh.mVertices);                // expect to have vertices
#if (XX_HAS_NORMALS())
    Panic(mesh.mNormals);                 //   and normals
#endif
#if (XX_HAS_TEXTURE_COORDS())
    Panic(mesh.mTextureCoords);           //   and texture coords
    Panic(mesh.mTextureCoords[0]);        //   ... one per vertex
    Panic(mesh.mNumUVComponents[0] == 2); //   that has only x and y.
    // A vertex can contain up to 8 different texture coordinates.
    // We expect to see only one for now.
    for (unsigned int i = 1; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i)
    {
        Panic(!mesh.mTextureCoords[i]);
    }
#endif

    Panic(mesh.mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

    AssimpMesh mesh_data{};
    mesh_data.texture_diffuse.material_index = static_cast<std::uint32_t>(-1);
    mesh_data.vertices.reserve(mesh.mNumVertices);
    mesh_data.indices.reserve(mesh.mNumFaces * 3u); // expects triangles

    for (unsigned int i = 0; i < mesh.mNumVertices; ++i)
    {
        mesh_data.vertices.push_back({});
        Vertex& v = mesh_data.vertices.back();

        v.position.x = mesh.mVertices[i].x;
        v.position.y = mesh.mVertices[i].y;
        v.position.z = mesh.mVertices[i].z;

#if (XX_HAS_NORMALS())
        v.normal.x = mesh.mNormals[i].x;
        v.normal.y = mesh.mNormals[i].y;
        v.normal.z = mesh.mNormals[i].z;
#else
        v.normal.x = 0.f;
        v.normal.y = 0.f;
        v.normal.z = 0.f;
#endif

#if (XX_HAS_TEXTURE_COORDS())
        v.texture_coord.x = mesh.mTextureCoords[0][i].x;
        v.texture_coord.y = mesh.mTextureCoords[0][i].y;
#else
        v.texture_coord.x = 0.f;
        v.texture_coord.y = 0.f;
#endif
    }

    static_assert(sizeof(Index) <= sizeof(unsigned int)
        , "Assimp supports indices up to unsigned int");

    for (unsigned int i = 0; i < mesh.mNumFaces; ++i)
    {
        Panic(mesh.mFaces[i].mNumIndices == 3); // triangles
        for (unsigned j = 0; j < mesh.mFaces[i].mNumIndices; ++j)
        {
            const unsigned int v = mesh.mFaces[i].mIndices[j];
            // Panic(static_cast<unsigned int>(std::numeric_limits<Index>::max()) >= v);
            mesh_data.indices.push_back(Index(v));
        }
    }

#if (XX_HAS_TEXTURE_COORDS())
    Panic(mesh.mMaterialIndex < scene.mNumMaterials);
    const aiMaterial& material = *scene.mMaterials[mesh.mMaterialIndex];

    const unsigned int count = material.GetTextureCount(aiTextureType_DIFFUSE);
    Panic(count == 1);
    aiString path;
    Panic(material.GetTexture(aiTextureType_DIFFUSE, 0, &path) == aiReturn_SUCCESS);
    mesh_data.texture_diffuse.material_index = mesh.mMaterialIndex;
    mesh_data.texture_diffuse.path.assign(path.C_Str());
    Panic(path.length == mesh_data.texture_diffuse.path.size());

    Panic(!mesh_data.texture_diffuse.path.empty());
#else
    (void)scene;
#endif

    Panic(!mesh_data.indices.empty());
    Panic(!mesh_data.vertices.empty());
    return mesh_data;
}

template<typename F>
static void Assimp_ProcessNode(const aiScene& scene, const aiNode& node, F on_new_mesh)
{
    for (unsigned int i = 0; i < node.mNumMeshes; ++i)
    {
        const aiMesh& mesh = *scene.mMeshes[node.mMeshes[i]];
        (void)on_new_mesh(Assimp_ProcessMesh(scene, mesh));
    }
    for (unsigned int i = 0; i < node.mNumChildren; ++i)
    {
        Assimp_ProcessNode(scene, *node.mChildren[i], on_new_mesh);
    }
}

static AssimpModel Assimp_Load(fs::path file_path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(file_path.string().c_str()
        , aiProcess_Triangulate);
    // aiProcess_FlipUVs - no need for DirectX.
    Panic(scene);
    Panic((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != AI_SCENE_FLAGS_INCOMPLETE);
    Panic(scene->mRootNode);

    const fs::path dir = file_path.parent_path();
    AssimpModel model;

    Assimp_ProcessNode(*scene, *scene->mRootNode
        , [&](AssimpMesh&& mesh)
    {
#if (XX_HAS_TEXTURE_COORDS())
        const fs::path texture_file = dir / mesh.texture_diffuse.path;
        const unsigned int material_index = mesh.texture_diffuse.material_index;
#endif

        model.meshes.push_back(std::move(mesh));

#if (XX_HAS_TEXTURE_COORDS())
        const auto it = std::find_if(std::cbegin(model.materials), std::cend(model.materials)
            , [&](const AssimpModel::Blob& data)
        {
            return (data.material_index == material_index);
        });
        if (it != std::cend(model.materials))
        {
            return;
        }

        const std::string path = texture_file.string();
        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        Panic(!!data);
        Panic(channels == c_texture_channels); // RGBA
        // stbi_image_free(data)
        auto& blob = model.materials.emplace_back(AssimpModel::Blob{});
        blob.data = data;
        blob.material_index = material_index;
        blob.width = static_cast<unsigned int>(width);
        blob.height = static_cast<unsigned int>(height);
#endif
    });

    return model;
}

int main()
{
#if (XX_HAS_TEXTURE_COORDS() && XX_HAS_NORMALS())
    const fs::path model_file = R"(K:\backpack\backpack.obj)";
#elif (XX_HAS_NORMALS())
    const fs::path model_file = R"(K:\skull\skull.obj)";
#else
    const fs::path model_file = R"(K:\tyrannosaurus-rex-skeleton\source\dyno_tex.obj)";
#endif

    AssimpModel model = Assimp_Load(model_file);
    Panic(!model.meshes.empty());
    
    Header header{};
    header.version_id   = std::uint16_t(0x2);
    header.meshes_count = std::uint32_t(model.meshes.size());
    header.textures_count = std::uint32_t(model.materials.size());

#if (XX_HAS_NORMALS())
    header.capabilitis |= std::uint16_t(Capabilities::Normals);
#endif
#if (XX_HAS_TEXTURE_COORDS())
    header.capabilitis |= std::uint16_t(Capabilities::TextureCoords);
    header.capabilitis |= std::uint16_t(Capabilities::TextureRGBA);
#endif

    std::uint32_t offset = 0
        + sizeof(header)                                 // Header
        + (sizeof(MeshData)    * header.meshes_count)    // Meshes
        + (sizeof(TextureData) * header.textures_count); // Textures

    std::vector<MeshData> meshes;
    for (const AssimpMesh& mesh : model.meshes)
    {
        MeshData data{};
        data.vertices_start = offset;
        offset += std::uint32_t(mesh.vertices.size() * sizeof(Vertex));
        data.vertices_end = offset;
        data.indices_start = offset;
        offset += std::uint32_t(mesh.indices.size() * sizeof(Index));
        data.indices_end = offset;
        data.texture_diffuse_id = std::uint32_t(mesh.texture_diffuse.material_index);
        meshes.push_back(data);
    }
    Panic(meshes.size() == model.meshes.size());

    std::vector<TextureData> textures;
    for (const AssimpModel::Blob& blob : model.materials)
    {
        const std::uint32_t size = (blob.height * blob.width * c_texture_channels); // RGBA

        TextureData data{};
        data.texture_id = blob.material_index;
        data.width = blob.width;
        data.height = blob.height;
        data.start = offset;
        offset += size;
        data.end = offset;
        textures.push_back(data);
    }
    Panic(textures.size() == model.materials.size());

    const std::uint32_t final_size = offset;
    offset = 0;

    fs::path bin_file = model_file.parent_path();
    bin_file /= model_file.stem().string() + ".lr.bin";

    FILE* f = nullptr;
    const errno_t e = fopen_s(&f, bin_file.string().c_str(), "wb");
    Panic(!e);

    // Header.
    std::uint32_t to_write = std::uint32_t(sizeof(header));
    Panic(fwrite(&header, 1, to_write, f) == to_write);
    offset += to_write;
    // Meshes.
    to_write = std::uint32_t(sizeof(MeshData) * header.meshes_count);
    Panic(fwrite(meshes.data(), 1, to_write, f) == to_write);
    offset += to_write;
    // Textures.
    to_write = std::uint32_t(sizeof(TextureData) * header.textures_count);
    Panic(fwrite(textures.data(), 1, to_write, f) == to_write);
    offset += to_write;

    // Binary data for Meshes.
    for (const AssimpMesh& mesh : model.meshes)
    {
        to_write = std::uint32_t(mesh.vertices.size() * sizeof(Vertex));
        Panic(fwrite(mesh.vertices.data(), 1, to_write, f) == to_write);
        offset += to_write;

        to_write = std::uint32_t(mesh.indices.size() * sizeof(Index));
        Panic(fwrite(mesh.indices.data(), 1, to_write, f) == to_write);
        offset += to_write;
    }

    // Binary data for Textures.
    for (AssimpModel::Blob& blob : model.materials)
    {
        to_write = std::uint32_t(blob.height * blob.width * c_texture_channels);
        Panic(fwrite(blob.data, 1, to_write, f) == to_write);
        offset += to_write;
        
        stbi_image_free(blob.data);
        blob.data = nullptr;
    }
    fclose(f);

    Panic(final_size == offset);
}
