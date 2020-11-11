#pragma once
#include "dx_api.h"
#include "utils.h"
#include "model.h"
#include "shaders_compiler.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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

#pragma warning(push)
// structure was padded due to alignment specifier
#pragma warning(disable:4324)
struct RenderModel
{
    // vs_basic_phong_lighting.hlsl
    struct VSConstantBuffer0
    {
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;
    };

    // ps_basic_phong_lighting.hlsl
    struct PSConstantBuffer0
    {
        glm::vec4 light_color;
        glm::vec4 light_position;
        glm::vec4 viewer_position;
        glm::vec4 has_texture;
    };

    const Model* model;
    std::vector<RenderMesh> meshes;
    std::vector<RenderTexture> textures;

    VSShader* vs_shader_ = nullptr;
    ComPtr<ID3D11Buffer> vs_constant_buffer0_;
    PSShader* ps_shader_ = nullptr;
    ComPtr<ID3D11Buffer> ps_constant_buffer0_;
    ComPtr<ID3D11SamplerState> sampler_linear_;

    // Tweak whole model position & orientation.
    DirectX::XMMATRIX world;

    // Tweak light.
    glm::vec3 light_color;
    glm::vec3 light_position;
    glm::vec3 viewer_position;

    static RenderModel make(ID3D11Device& device, const Model& model);

    void render(ID3D11DeviceContext& device_context
        , const DirectX::XMMATRIX& view
        , const DirectX::XMMATRIX& projection) const;
};
#pragma warning(pop)
