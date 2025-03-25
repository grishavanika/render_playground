#pragma once
#include "dx_api.h"
#include "utils.h"
#include "model.h"
#include "shaders_compiler.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vector>

struct RenderMesh
{
    ComPtr<ID3D11Buffer> vertex_buffer;
    ComPtr<ID3D11Buffer> index_buffer;
    UINT indices_count;
    std::uint32_t ps_texture_diffuse;
    std::uint32_t ps_texture_normal;

    static RenderMesh make(ID3D11Device& device, const Mesh& mesh);
};

struct RenderTexture
{
    ComPtr<ID3D11ShaderResourceView> texture_view;
    std::uint32_t texture_id;

    static RenderTexture make(ID3D11Device& device, const Texture& texture);
};

struct RenderModel
{
    // vs_basic_phong_lighting.hlsl
    struct VSConstantBuffer0
    {
        glm::mat4x4 world;
        glm::mat4x4 view;
        glm::mat4x4 projection;
    };

    // ps_basic_phong_lighting.hlsl
    struct PointLight
    {
        glm::vec4 light_color;
        glm::vec4 light_position;
    };

    static constexpr unsigned k_max_lights_count = 16;

    struct PSConstantBuffer0
    {
        PointLight lights[k_max_lights_count];
        glm::vec4 viewer_position;
        // x = has textures
        // y = lights count
        glm::vec4 parameters;
    };

    std::vector<RenderMesh> meshes;
    std::vector<RenderTexture> textures;

    VSShader* vs_shader_ = nullptr;
    ComPtr<ID3D11Buffer> vs_constant_buffer0_;
    PSShader* ps_shader_ = nullptr;
    ComPtr<ID3D11Buffer> ps_constant_buffer0_;
    ComPtr<ID3D11SamplerState> sampler_linear_;

    // Tweak whole model position & orientation.
    glm::mat4x4 world;

    // Tweak light.
    glm::vec3 light_color;
    glm::vec3 light_position;
    glm::vec3 viewer_position;

    static RenderModel make(ID3D11Device& device, const Model& model);

    void render(ID3D11DeviceContext& device_context
        , const glm::mat4x4& view
        , const glm::mat4x4& projection) const;
};
