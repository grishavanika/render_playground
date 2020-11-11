#include "model.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/vec3.hpp>

#include <vector>
#include <filesystem>
#include <algorithm>

#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <cstdio>

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
    std::string path;
};

struct AssimpMesh
{
    std::vector<Vertex> vertices;
    std::vector<Index> indices;
    AssimpTexture texture_diffuse;
    AssimpTexture texture_normal;
    bool has_normals = false;
    bool has_texture_coords = false;
    glm::vec3 aabb_min;
    glm::vec3 aabb_max;
};

struct AssimpModel
{
    struct Blob
    {
        std::string path; // not exported
        unsigned char* data; // stbi_image_free(data)
        unsigned int width;
        unsigned int height;
    };
    std::vector<AssimpMesh> meshes;
    std::vector<Blob> materials;
    glm::vec3 aabb_min;
    glm::vec3 aabb_max;
};

// Model's loading with Assimp comes from learnopengl.com:
// https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/model.h
static AssimpMesh Assimp_ProcessMesh(const aiScene& scene, const aiMesh& mesh)
{
    static_assert(std::is_same_v<float, ai_real>);

    AssimpMesh mesh_data{};

    Panic(mesh.mVertices);                   // expect to have vertices
    mesh_data.has_normals = !!mesh.mNormals; // normals are optional
    mesh_data.has_texture_coords = [&]()
    {
        // #QQQ: clean-up - better names and document the logic behind this.
        const bool has_uv = mesh.mTangents      // valid tangents
            && mesh.mTextureCoords              // and texture coords
            && mesh.mTextureCoords[0]           // ... one per vertex
            && (mesh.mNumUVComponents[0] == 2)  // that has only x and y.
            && (mesh.mMaterialIndex < scene.mNumMaterials)
            && (scene.mMaterials[mesh.mMaterialIndex]->GetTextureCount(aiTextureType_DIFFUSE) == 1);
        if (has_uv)
        {
            // A vertex can contain up to 8 different texture coordinates.
            // We expect to see only one for now.
            for (unsigned int i = 1; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i)
            {
                Panic(!mesh.mTextureCoords[i]);
            }
        }
        return has_uv;
    }();

    Panic(mesh.mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

    mesh_data.vertices.reserve(mesh.mNumVertices);
    mesh_data.indices.reserve(mesh.mNumFaces * 3u); // expects triangles
    
    mesh_data.aabb_min.x = mesh.mAABB.mMin.x;
    mesh_data.aabb_min.y = mesh.mAABB.mMin.y;
    mesh_data.aabb_min.z = mesh.mAABB.mMin.z;
    mesh_data.aabb_max.x = mesh.mAABB.mMax.x;
    mesh_data.aabb_max.y = mesh.mAABB.mMax.y;
    mesh_data.aabb_max.z = mesh.mAABB.mMax.z;

    for (unsigned int i = 0; i < mesh.mNumVertices; ++i)
    {
        mesh_data.vertices.push_back({});
        Vertex& v = mesh_data.vertices.back();

        v.position.x = mesh.mVertices[i].x;
        v.position.y = mesh.mVertices[i].y;
        v.position.z = mesh.mVertices[i].z;

        if (mesh_data.has_normals)
        {
            v.normal.x = mesh.mNormals[i].x;
            v.normal.y = mesh.mNormals[i].y;
            v.normal.z = mesh.mNormals[i].z;
        }
        if (mesh_data.has_texture_coords)
        {
            v.texture_coord.x = mesh.mTextureCoords[0][i].x;
            v.texture_coord.y = mesh.mTextureCoords[0][i].y;
            v.tangent.x = mesh.mTangents[i].x;
            v.tangent.y = mesh.mTangents[i].y;
            v.tangent.z = mesh.mTangents[i].z;
        }
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

    if (mesh_data.has_texture_coords)
    {
        Panic(mesh.mMaterialIndex < scene.mNumMaterials);
        const aiMaterial& material = *scene.mMaterials[mesh.mMaterialIndex];

        {
            const unsigned int count = material.GetTextureCount(aiTextureType_DIFFUSE);
            Panic(count == 1);
            aiString path;
            Panic(material.GetTexture(aiTextureType_DIFFUSE, 0, &path) == aiReturn_SUCCESS);
            mesh_data.texture_diffuse.path.assign(path.C_Str());
            Panic(path.length == mesh_data.texture_diffuse.path.size());

            Panic(!mesh_data.texture_diffuse.path.empty());
        }

        {
            const unsigned int count = material.GetTextureCount(aiTextureType_HEIGHT);
            Panic(count == 1);
            aiString path;
            Panic(material.GetTexture(aiTextureType_HEIGHT, 0, &path) == aiReturn_SUCCESS);
            mesh_data.texture_normal.path.assign(path.C_Str());
            Panic(path.length == mesh_data.texture_normal.path.size());

            Panic(!mesh_data.texture_normal.path.empty());
        }
    }

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

static void UpdateAABB(AssimpModel& model, const AssimpMesh& mesh)
{
    if (mesh.aabb_min.x < model.aabb_min.x) model.aabb_min.x = mesh.aabb_min.x;
    if (mesh.aabb_min.y < model.aabb_min.y) model.aabb_min.y = mesh.aabb_min.y;
    if (mesh.aabb_min.z < model.aabb_min.z) model.aabb_min.z = mesh.aabb_min.z;

    if (mesh.aabb_max.x > model.aabb_max.x) model.aabb_max.x = mesh.aabb_max.x;
    if (mesh.aabb_max.y > model.aabb_max.y) model.aabb_max.y = mesh.aabb_max.y;
    if (mesh.aabb_max.z > model.aabb_max.z) model.aabb_max.z = mesh.aabb_max.z;
}

static AssimpModel Assimp_Load(fs::path file_path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(file_path.string().c_str()
        , unsigned(
            aiProcess_Triangulate
            | aiProcess_CalcTangentSpace
            | aiProcess_GenBoundingBoxes));
    // aiProcess_FlipUVs - no need for DirectX.
    Panic(scene);
    Panic((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != AI_SCENE_FLAGS_INCOMPLETE);
    Panic(scene->mRootNode);

    const fs::path dir = file_path.parent_path();
    AssimpModel model;
    model.aabb_min = glm::vec3(FLT_MAX);
    model.aabb_max = glm::vec3(FLT_MIN);

    Assimp_ProcessNode(*scene, *scene->mRootNode
        , [&](AssimpMesh&& mesh)
    {
        if (mesh.has_texture_coords)
        {
            const AssimpTexture textures[2] = {mesh.texture_diffuse, mesh.texture_normal};

            for (const AssimpTexture& t : textures)
            {
                const fs::path texture_file = dir / t.path;
                const auto it = std::find_if(std::cbegin(model.materials), std::cend(model.materials)
                    , [&](const AssimpModel::Blob& data)
                    {
                        return (data.path == t.path);
                    });
                if (it != std::cend(model.materials))
                {
                    continue;
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
                blob.path = t.path;
                blob.width = static_cast<unsigned int>(width);
                blob.height = static_cast<unsigned int>(height);
            }
        }

        UpdateAABB(model, mesh);
        model.meshes.push_back(std::move(mesh));
    });

    return model;
}

#if !defined(XX_ASSETS_FOLDER)
#  error "Build system missed to specify where raw assets are."
#endif

std::uint16_t GetAllMeshesCapabilities(const std::vector<AssimpMesh>& meshes)
{
    Panic(!meshes.empty());
    std::uint16_t capabilitis = std::uint16_t(Capabilities::Default);
    const bool has_normals = meshes[0].has_normals;
    const bool has_texture_coords = meshes[0].has_texture_coords;
    
    const bool all_same = std::all_of(meshes.cbegin(), meshes.cend()
        , [&](const AssimpMesh& mesh)
    {
        return (mesh.has_normals == has_normals)
            && (mesh.has_texture_coords == has_texture_coords);
    });
    Panic(all_same);

    if (has_normals)
    {
        capabilitis |= std::uint16_t(Capabilities::Normals);
    }
    if (has_texture_coords)
    {
        capabilitis |= std::uint16_t(Capabilities::TextureCoords);
        capabilitis |= std::uint16_t(Capabilities::TextureRGBA);
        capabilitis |= std::uint16_t(Capabilities::Tangents);
    }

    return capabilitis;
}

static void LogCapabilities(std::uint16_t capabilities)
{
    const struct
    {
        const Capabilities flag;
        const char* const description;
    } info[] =
    {
        {Capabilities::Default,         "vertices with faces/triangles"},
        {Capabilities::Normals,         "+ normal per vertex"},
        {Capabilities::TextureCoords,   "+ 2D UV texture mapping"},
        {Capabilities::TextureRGBA,     "texture is RGBA, 8 bit per channel, sRGB"},
        {Capabilities::Tangents,        "+ tangents"},
    };
    for (const auto& data : info)
    {
        if ((capabilities & std::uint16_t(data.flag)) == std::uint16_t(data.flag))
        {
            std::printf("    %s\n", data.description);
        }
    }
}

int main(int argc, char* argv[])
{
    Panic(argc >= 3);
    const fs::path model_file = argv[2];
    const char* output_path = argv[1];

    std::printf("Loading '%s' file.\n", model_file.string().c_str());

    AssimpModel model = Assimp_Load(model_file);
    Panic(!model.meshes.empty());
    
    Header header{};
    header.version_id   = std::uint16_t(Header::k_current_version);
    header.meshes_count = std::uint32_t(model.meshes.size());
    header.textures_count = std::uint32_t(model.materials.size());
    header.capabilitis = GetAllMeshesCapabilities(model.meshes);
    header.aabb_min = model.aabb_min;
    header.aabb_max = model.aabb_max;

    std::uint32_t offset = 0
        + sizeof(header)                                 // Header
        + (sizeof(MeshData)    * header.meshes_count)    // Meshes
        + (sizeof(TextureData) * header.textures_count); // Textures

    auto get_texture_id = [&](const std::string& path)
    {
        for (std::uint32_t i = 0, count = std::uint32_t(model.materials.size()); i < count; ++i)
        {
            if (model.materials[i].path == path)
            {
                return (i + 1);
            }
        }
        return std::uint32_t(-1);
    };

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
        data.texture_diffuse_id = get_texture_id(mesh.texture_diffuse.path);
        data.texture_normal_id = get_texture_id(mesh.texture_normal.path);
        meshes.push_back(data);
    }
    Panic(meshes.size() == model.meshes.size());

    std::vector<TextureData> textures;
    for (std::uint32_t i = 0, count = std::uint32_t(model.materials.size()); i < count; ++i)
    {
        const AssimpModel::Blob& blob = model.materials[i];
        const std::uint32_t size = (blob.height * blob.width * c_texture_channels); // RGBA

        TextureData data{};
        data.texture_id = (i + 1);
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

    fs::path bin_file = output_path;
    bin_file /= (model_file.stem().string() + ".lr.bin");
    const std::string binary_file = bin_file.string();

    std::printf("Writing to '%s' file.\n", binary_file.c_str());

    FILE* f = nullptr;
    const errno_t e = fopen_s(&f, binary_file.c_str(), "wb");
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

    std::printf("Done.\n");
    LogCapabilities(header.capabilitis);
}
