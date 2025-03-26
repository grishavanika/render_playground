#include "assimp_model.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/vec3.hpp>

#include <algorithm>
#include <filesystem>
#include <vector>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace fs = std::filesystem;

static void Panic(bool condition)
{
    if (!condition)
    {
        assert(false);
        std::exit(1);
    }
}

// Model's loading with Assimp comes from learnopengl.com:
// https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/model.h
static AssimpMesh Assimp_ProcessMesh(const aiScene& scene, const aiMesh& mesh)
{
    static_assert(std::is_same_v<float, ai_real>);

    AssimpMesh mesh_data{};

    Panic(mesh.mVertices);                   // expect to have vertices
    mesh_data.has_normals = !!mesh.mNormals; // normals are optional
    mesh_data.has_texture_coords = [&]() {
        // #QQQ: clean-up - better names and document the logic behind this.
        const bool has_uv = mesh.mTangents                                 // valid tangents
                            && mesh.mTextureCoords[0]                      // ... one per vertex
                            && (mesh.mNumUVComponents[0] == 2)             // that has only x and y.
                            && (mesh.mMaterialIndex < scene.mNumMaterials) //
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

    static_assert(sizeof(Index) <= sizeof(unsigned int), "Assimp supports indices up to unsigned int");

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

template <typename F>
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
    if (mesh.aabb_min.x < model.aabb_min.x)
    {
        model.aabb_min.x = mesh.aabb_min.x;
    }
    if (mesh.aabb_min.y < model.aabb_min.y)
    {
        model.aabb_min.y = mesh.aabb_min.y;
    }
    if (mesh.aabb_min.z < model.aabb_min.z)
    {
        model.aabb_min.z = mesh.aabb_min.z;
    }

    if (mesh.aabb_max.x > model.aabb_max.x)
    {
        model.aabb_max.x = mesh.aabb_max.x;
    }
    if (mesh.aabb_max.y > model.aabb_max.y)
    {
        model.aabb_max.y = mesh.aabb_max.y;
    }
    if (mesh.aabb_max.z > model.aabb_max.z)
    {
        model.aabb_max.z = mesh.aabb_max.z;
    }
}

/*static*/ AssimpModel Assimp_Load(fs::path file_path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        file_path.string().c_str(),
        unsigned(aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenBoundingBoxes)
    );
    // aiProcess_FlipUVs - no need for DirectX.
    Panic(scene);
    Panic((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != AI_SCENE_FLAGS_INCOMPLETE);
    Panic(scene->mRootNode);

    const fs::path dir = file_path.parent_path();
    AssimpModel model;
    model.aabb_min = glm::vec3(FLT_MAX);
    model.aabb_max = glm::vec3(FLT_MIN);

    Assimp_ProcessNode(*scene, *scene->mRootNode, [&](AssimpMesh&& mesh) {
        if (mesh.has_texture_coords)
        {
            const AssimpTexture textures[2] = {mesh.texture_diffuse, mesh.texture_normal};

            for (const AssimpTexture& t : textures)
            {
                const fs::path texture_file = dir / t.path;
                const auto it = std::find_if(
                    std::cbegin(model.materials),
                    std::cend(model.materials),
                    [&](const AssimpModel::Blob& data) { return (data.path == t.path); }
                );
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
