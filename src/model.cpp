#include "model.h"
#include "assimp_model.h"

#include <cassert>
#include <cstdio>

static void Panic(bool condition)
{
    if (!condition)
    {
        assert(false);
        std::exit(1);
    }
}

Model::Model() noexcept = default;
Model::~Model() noexcept = default;
Model::Model(Model&&) noexcept = default;
Model& Model::operator=(Model&&) noexcept = default;

glm::vec3 Model::aabb_min() const
{
    Panic(!!assimp_);
    return assimp_->aabb_min;
}

glm::vec3 Model::aabb_max() const
{
    Panic(!!assimp_);
    return assimp_->aabb_max;
}

std::uint32_t Model::meshes_count() const
{
    Panic(!!assimp_);
    return std::uint32_t(assimp_->meshes.size());
}

std::uint32_t Model::textures_count() const
{
    Panic(!!assimp_);
    return std::uint32_t(assimp_->materials.size());
}

Mesh Model::get_mesh(std::uint32_t index) const
{
    Panic(!!assimp_);
    Panic(index < assimp_->meshes.size());
    const AssimpMesh& assimp_mesh = assimp_->meshes[index];

    auto get_texture_id = [&](const std::string& path) {
        for (std::uint32_t i = 0, count = std::uint32_t(assimp_->materials.size()); i < count; ++i)
        {
            if (assimp_->materials[i].path == path)
            {
                return i;
            }
        }
        return std::uint32_t(-1);
    };

    Mesh mesh{};
    mesh.vertices = assimp_mesh.vertices;
    mesh.indices = assimp_mesh.indices;
    mesh.texture_diffuse_id = get_texture_id(assimp_mesh.texture_diffuse.path);
    mesh.texture_normal_id = get_texture_id(assimp_mesh.texture_normal.path);
    return mesh;
}

Texture Model::get_texture(std::uint32_t index) const
{
    Panic(!!assimp_);
    const AssimpModel::Blob& assimp_texture = assimp_->materials[index];

    const std::size_t size = assimp_texture.height * assimp_texture.width * c_texture_channels;
    Texture texture{};
    texture.id = index;
    texture.height = assimp_texture.height;
    texture.width = assimp_texture.width;
    texture.data = {assimp_texture.data, assimp_texture.data + size};
    return texture;
}

outcome::result<Model> LoadModel(const char* filename)
{
    AssimpModel model = Assimp_Load(filename);
    Panic(!model.meshes.empty());
    Model m{};
    m.assimp_ = std::make_unique<AssimpModel>(std::move(model));
    return outcome::success(std::move(m));
}
