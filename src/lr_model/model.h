#pragma once
#include "vertex.h"

#if (__has_include(<span>))
#  include <span>
#else
// Narrow case only to MinGW to control explicitly where this hack exists
#if (defined(__MINGW32__) || defined(__MINGW64__))
#include <cstddef>
// Introducing std:: intentionally. This should be cut anyway once MinGW gains <span>
namespace std
{
    template<typename T>
    class span
    {
    public:
        constexpr span() noexcept
            : span(nullptr, 0)
        {
        }

        constexpr span(T* ptr, std::size_t count) noexcept
            : ptr_(ptr)
            , count_(count)
        {
        }

        constexpr T& operator[](std::size_t i) const noexcept
        {
            return ptr_[i];
        }

    private:
        T* ptr_;
        std::size_t count_;
    };
}
#endif
#endif

#include <type_traits>

#include <cstdint>

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
    std::uint32_t meshes_count_;
    std::uint32_t textures_count_;
    std::uint32_t memory_size_;
    const std::uint8_t* memory_;
    Vector3f aabb_min_{};
    Vector3f aabb_max_{};

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
    Tangents      = 0x8
};

struct Header
{
    static constexpr std::uint16_t k_current_version = 0x6;

    std::uint16_t version_id;
    std::uint16_t capabilitis;
    std::uint32_t meshes_count;
    std::uint32_t textures_count;

    Vector3f aabb_min;
    Vector3f aabb_max;
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
