#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <glm/vec3.hpp>
#include "model.h"

namespace fs = std::filesystem;

struct AssimpModel;
AssimpModel Assimp_Load(fs::path file_path);

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

