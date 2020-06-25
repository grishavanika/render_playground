#pragma once
#pragma warning(push)
// macro redefinition
#pragma warning(disable : 4005)
#include <d3d11.h>
#pragma warning(pop)
#include <DirectXMath.h>

#include <vector>
#include <span>

#include "utils.h"
#include "vertex.h"
#include "shaders_compiler.h"

struct RenderWithNormals
{
    struct NormalsVertex
    {
        Vector3f position;
        Vector3f normal;
    };

    std::vector<NormalsVertex> vertices_;

    ComPtr<ID3D11Device> device_ = nullptr;
    ComPtr<ID3D11VertexShader> vertex_shader_ = nullptr;
    ComPtr<ID3D11InputLayout> vertex_layout_ = nullptr;
    ComPtr<ID3D11PixelShader> pixel_shader_ = nullptr;
    ComPtr<ID3D11Buffer> vertex_buffer_ = nullptr;
    ComPtr<ID3D11Buffer> constant_buffer_ = nullptr;

    DirectX::XMMATRIX world;

    static RenderWithNormals make(const ComPtr<ID3D11Device>& device
        , const std::span<const NormalsVertex>& vertices);

    void render(ID3D11DeviceContext& device_context
        , const DirectX::XMMATRIX& view_transposed
        , const DirectX::XMMATRIX& projection_transposed) const;
};
