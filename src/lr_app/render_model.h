#pragma once
#pragma warning(push)
// macro redefinition
#pragma warning(disable : 4005)
#include <d3d11.h>
#pragma warning(pop)

#include <DirectXMath.h>

#include <vector>

#include "utils.h"
#include "model.h"
#include "shaders_compiler.h"

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
        DirectX::XMVECTOR light_color;
        DirectX::XMVECTOR light_position;
        DirectX::XMVECTOR viewer_position;
    };

    const Model* model;
    std::vector<RenderMesh> meshes;
    std::vector<RenderTexture> textures;

    ComPtr<ID3D11InputLayout> vertex_layout_;
    ComPtr<ID3D11VertexShader> vertex_shader_;
    ComPtr<ID3D11Buffer> vs_constant_buffer0_;
    ComPtr<ID3D11PixelShader> pixel_shader_;
    ComPtr<ID3D11Buffer> ps_constant_buffer0_;
    ComPtr<ID3D11SamplerState> sampler_linear_;

    // Tweak whole model position & orientation.
    DirectX::XMMATRIX world;

    // Tweak light.
    DirectX::XMVECTOR light_color;
    DirectX::XMVECTOR light_position;
    DirectX::XMVECTOR viewer_position;

    static RenderModel make(ID3D11Device& device, const Model& model);

    void render(ID3D11DeviceContext& device_context
        , const DirectX::XMMATRIX& view_transposed
        , const DirectX::XMMATRIX& projection_transposed) const;
};
#pragma warning(pop)
