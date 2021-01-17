#pragma once
#include "vertex.h"
#include "utils_outcome.h"

#include <glm/vec3.hpp>

#include <span>
#include <type_traits>

#include <cstdint>

struct Model;
outcome::result<Model> LoadModel(const char* filename);

// RGBA, 8 bits per channel.
// In the example (backpack/diffuse.png) it's actually
// DXGI_FORMAT_R8G8B8A8_UNORM_SRGB.
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
    std::uint32_t meshes_count_ = 0;
    std::uint32_t textures_count_ = 0;
    std::uint32_t memory_size_ = 0;
    const std::uint8_t* memory_ = nullptr;
    glm::vec3 aabb_min_{};
    glm::vec3 aabb_max_{};

    Mesh get_mesh(std::uint32_t index) const;
    Texture get_texture(std::uint32_t index) const;

    Model() = default;
    ~Model();
    Model(const Model& rhs) = delete;
    Model& operator=(const Model& rhs) = delete;
    Model& operator=(Model&& rhs) noexcept;
    Model(Model&& rhs) noexcept;
};

struct FileModel
{
    std::string file_name;
    std::string name;
    Model model;
};

enum struct Capabilities : std::uint16_t
{
    Default       = 0x0, // Minimum needed: vertices with faces/triangles
    Normals       = 0x1, //  + normal per vertex
    TextureCoords = 0x2, //  + 2D UV texture mapping
    TextureRGBA   = 0x4, // texture is RGBA, 8 bit per channel, sRGB now.
    Tangents      = 0x8
};

struct Header
{
    static constexpr std::uint16_t k_current_version = 0x6;

    std::uint16_t version_id;
    std::uint16_t capabilitis;
    std::uint32_t meshes_count;
    std::uint32_t textures_count;

    glm::vec3 aabb_min;
    glm::vec3 aabb_max;
};
static_assert(sizeof(Header) == 36);
static_assert(std::is_trivial<Header>());

struct MeshData
{
    std::uint32_t vertices_start;
    std::uint32_t vertices_end;
    std::uint32_t indices_start;
    std::uint32_t indices_end;
    std::uint32_t texture_diffuse_id;
    std::uint32_t texture_normal_id;
};
static_assert(sizeof(MeshData) == 24);
static_assert(std::is_trivial<MeshData>());

constexpr std::uint32_t c_texture_channels = 4; // RGBA, 8 bit per channel.

struct TextureData
{
    std::uint32_t texture_id;
    std::uint32_t start;
    std::uint32_t end;
    std::uint32_t width;
    std::uint32_t height;
};
static_assert(sizeof(TextureData) == 20);
static_assert(std::is_trivial<TextureData>());
