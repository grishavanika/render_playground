#pragma once
#include "vertex.h"

#include <span>
#include <type_traits>

#include <cstdint>

#define XX_HAS_TEXTURE_COORDS() 1
#define XX_HAS_NORMALS() 1

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
};

struct Model
{
    std::uint32_t meshes_count_;
    std::uint32_t textures_count_;
    std::uint32_t memory_size_;
    const std::uint8_t* memory_;

    Mesh get_mesh(std::uint32_t index) const;
    Texture get_texture(std::uint32_t index) const;

    Model() = default;
    ~Model();
    Model(const Model& rhs) = delete;
    Model& operator=(const Model& rhs) = delete;
    Model& operator=(Model&& rhs) = delete;
    Model(Model&& rhs) noexcept;
};

Model LoadModel(const char* filename);

enum struct Capabilities : std::uint16_t
{
    Default       = 0x0, // Minimum needed: vertices with faces/triangles
    Normals       = 0x1, //  + normal per vertex
    TextureCoords = 0x2, //  + 2D UV texture mapping
    TextureRGBA   = 0x4, // texture is RGBA, 8 bit per channel, sRGB now.
};

struct Header
{
    std::uint16_t version_id;
    std::uint16_t capabilitis;
    std::uint32_t meshes_count;
    std::uint32_t textures_count;
};
static_assert(sizeof(Header) == 12);
static_assert(std::is_trivial<Header>());

struct MeshData
{
    std::uint32_t vertices_start;
    std::uint32_t vertices_end;
    std::uint32_t indices_start;
    std::uint32_t indices_end;
    std::uint32_t texture_diffuse_id;
};
static_assert(sizeof(MeshData) == 20);
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
