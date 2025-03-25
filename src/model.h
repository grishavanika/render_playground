#pragma once
#include "vertex.h"
#include "utils_outcome.h"

#include <glm/vec3.hpp>

#include <span>
#include <type_traits>

#include <cstdint>

struct AssimpModel;
struct Model;
outcome::result<Model> LoadModel(const char* filename);

// RGBA, 8 bits per channel.
// In the example (backpack/diffuse.png) it's actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB.
// No mips.
struct Texture
{
    std::uint32_t id;
    std::span<const std::uint8_t> data;
    std::uint32_t width;
    std::uint32_t height;
};

struct Mesh
{
    std::span<const Vertex> vertices;
    std::span<const Index> indices;
    std::uint32_t texture_diffuse_id;
    std::uint32_t texture_normal_id;
};

struct Model
{
    std::unique_ptr<AssimpModel> assimp_;

    Mesh get_mesh(std::uint32_t index) const;
    Texture get_texture(std::uint32_t index) const;

    glm::vec3 aabb_min() const;
    glm::vec3 aabb_max() const;
    std::uint32_t meshes_count() const;
    std::uint32_t textures_count() const;

    Model() noexcept;
    Model(Model&&) noexcept;
    Model& operator=(Model&&) noexcept;
    ~Model() noexcept;
};

struct FileModel
{
    std::string file_name;
    std::string name;
    Model model;
};

constexpr std::uint32_t c_texture_channels = 4; // RGBA, 8 bit per channel.
