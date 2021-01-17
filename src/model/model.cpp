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
    , aabb_min_(rhs.aabb_min_)
    , aabb_max_(rhs.aabb_max_)
{
    rhs.memory_ = nullptr;
    rhs.memory_size_ = 0;
    rhs.meshes_count_ = 0;
    rhs.textures_count_ = 0;
}

Model& Model::operator=(Model&& rhs) noexcept
{
    if (this != &rhs)
    {
        this->~Model();
        new(this) Model(std::move(rhs));
    }
    return *this;
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
    mesh.texture_normal_id = mesh_data.texture_normal_id;
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

outcome::result<Model> LoadModel(const char* filename)
{
    void* data = nullptr;
    size_t size = 0;
    {
        FILE* f = nullptr;
        const errno_t e = fopen_s(&f, filename, "rb");
        if (e) return outcome::failure(std::error_code(int(e), std::system_category()));
        int set = fseek(f, 0, SEEK_END);
        if (set != 0) return outcome::failure(todo::not_implemented);
        const long int ssize = ftell(f);
        if (ssize == -1)  return outcome::failure(todo::not_implemented);
        size = size_t(ssize);
        set = fseek(f, 0, SEEK_SET);
        if (set != 0) return outcome::failure(todo::not_implemented);
        data = malloc(size);
        if (!data) return outcome::failure(todo::not_implemented);
        size_t actually_read = fread(data, 1, size, f);
        fclose(f);
        if (actually_read != size) return outcome::failure(todo::not_implemented);
    }

    // UB everywhere.
    std::uint32_t needed_size = sizeof(Header);
    if (size <= needed_size) return outcome::failure(todo::not_implemented);
    const Header* header = static_cast<const Header*>(data);
    if (header->version_id != Header::k_current_version) return outcome::failure(todo::not_implemented);
    if (header->meshes_count == 0) return outcome::failure(todo::not_implemented);
    if ((header->capabilitis & std::uint16_t(Capabilities::TextureCoords))
        && (header->textures_count == 0))
    {
        return outcome::failure(todo::not_implemented);
    }

    needed_size += std::uint32_t(sizeof(MeshData) * header->meshes_count);
    if (size <= needed_size) return outcome::failure(todo::not_implemented);
    needed_size += std::uint32_t(sizeof(TextureData) * header->textures_count);
    if (size <= needed_size) return outcome::failure(todo::not_implemented);

    const std::uint8_t* start = static_cast<const std::uint8_t*>(data);
    Model m{};
    m.aabb_min_ = header->aabb_min;
    m.aabb_max_ = header->aabb_max;
    m.memory_ = start;
    m.meshes_count_ = header->meshes_count;
    m.textures_count_ = header->textures_count;
    m.memory_size_ = std::uint32_t(size);
    return outcome::success(std::move(m));
}
