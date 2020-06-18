#include "model.h"

#include <cstdio>
#include <cassert>

static void Panic(bool condition)
{
    if (!condition)
    {
        assert(false);
        std::exit(1);
    }
}

Model::Model(Model&& rhs) noexcept
    : meshes_count_(rhs.meshes_count_)
    , textures_count_(rhs.textures_count_)
    , memory_size_(rhs.memory_size_)
    , memory_(rhs.memory_)
{
    rhs.memory_ = nullptr;
    rhs.memory_size_ = 0;
    rhs.meshes_count_ = 0;
    rhs.textures_count_ = 0;
}

Model::~Model()
{
    if (memory_)
    {
        free((void*)memory_);
    }
    memory_ = nullptr;
    memory_size_ = 0;
    meshes_count_ = 0;
    textures_count_ = 0;
}

Mesh Model::get_mesh(std::uint32_t index) const
{
    Panic(!!memory_);
    Panic(index < meshes_count_);
    // UB everywhere.

    const std::uint32_t offset = sizeof(Header);
    Panic(memory_size_ > offset);

    const std::span<const MeshData> meshes(
        reinterpret_cast<const MeshData*>(memory_ + offset)
        , meshes_count_);
    const MeshData& mesh_data = meshes[index];

    Panic(memory_size_ >= mesh_data.vertices_start);
    Panic(memory_size_ >= mesh_data.vertices_end);
    Panic(memory_size_ >= mesh_data.indices_start);
    Panic(memory_size_ >= mesh_data.indices_end);

    const std::span<const Vertex> vertices(
        reinterpret_cast<const Vertex*>(memory_ + mesh_data.vertices_start)
        , (mesh_data.vertices_end - mesh_data.vertices_start) / sizeof(Vertex));
    const std::span<const Index> indices(
        reinterpret_cast<const Index*>(memory_ + mesh_data.indices_start)
        , (mesh_data.indices_end - mesh_data.indices_start) / sizeof(Index));

    Mesh mesh{};
    mesh.vertices = vertices;
    mesh.indices = indices;
    mesh.texture_diffuse_id = mesh_data.texture_diffuse_id;
    return mesh;
}

Texture Model::get_texture(std::uint32_t index) const
{
    Panic(!!memory_);
    Panic(index < textures_count_);
    // UB everywhere.

    const std::uint32_t offset = 0
        + sizeof(Header)
        + (sizeof(MeshData) * meshes_count_);
    Panic(memory_size_ > offset);

    const std::span<const TextureData> textures(
        reinterpret_cast<const TextureData*>(memory_ + offset)
        , textures_count_);
    const TextureData& mesh_data = textures[index];

    const std::span<const std::uint8_t> data(
        reinterpret_cast<const std::uint8_t*>(memory_ + mesh_data.start)
        , (mesh_data.end - mesh_data.start) / sizeof(std::uint8_t));

    Texture texture{};
    texture.id = mesh_data.texture_id;
    texture.height = mesh_data.height;
    texture.width = mesh_data.width;
    texture.data = data;
    return texture;
}

Model LoadModel(const char* filename)
{
    FILE* f = nullptr;
    const errno_t e = fopen_s(&f, filename, "rb");
    Panic(!e);
    Panic(fseek(f, 0, SEEK_END) == 0);
    const long int ssize = ftell(f);
    Panic(ssize != -1);
    const size_t size = size_t(ssize);
    Panic(fseek(f, 0, SEEK_SET) == 0);
    void* data = malloc(size);
    Panic(!!data);
    Panic(fread(data, 1, size, f) == size);
    fclose(f);

    // UB everywhere.
    std::uint32_t needed_size = sizeof(Header);
    Panic(size > needed_size);
    const Header* header = static_cast<const Header*>(data);
    Panic(header->version_id == 0x2);

    std::uint16_t capabilitis = std::uint16_t(Capabilities::Default);
#if (XX_HAS_NORMALS())
    capabilitis |= std::uint16_t(Capabilities::Normals);
#endif
#if (XX_HAS_TEXTURE_COORDS())
    capabilitis |= std::uint16_t(Capabilities::TextureCoords);
    capabilitis |= std::uint16_t(Capabilities::TextureRGBA);
#endif
    Panic(header->capabilitis == capabilitis);
    Panic(header->meshes_count > 0);
#if (XX_HAS_TEXTURE_COORDS())
    Panic(header->textures_count > 0);
#endif

    needed_size += std::uint32_t(sizeof(MeshData) * header->meshes_count);
    Panic(size > needed_size);
    needed_size += std::uint32_t(sizeof(TextureData) * header->textures_count);
    Panic(size > needed_size);

    const std::uint8_t* start = static_cast<const std::uint8_t*>(data);
    Model m{};
    m.memory_ = start;
    m.meshes_count_ = header->meshes_count;
    m.textures_count_ = header->textures_count;
    m.memory_size_ = std::uint32_t(size);
    return m;
}
