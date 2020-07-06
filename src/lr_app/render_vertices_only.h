#pragma once
#include "dx_api.h"

#include <vector>
#include <span>

#include "utils.h"
#include "vertex.h"
#include "shaders_compiler.h"

struct RenderVertices
{
    std::vector<Vector3f> vertices_;

    ComPtr<ID3D11Device> device_ = nullptr;
    ComPtr<ID3D11VertexShader> vertex_shader_ = nullptr;
    ComPtr<ID3D11InputLayout> vertex_layout_ = nullptr;
    ComPtr<ID3D11PixelShader> pixel_shader_ = nullptr;
    ComPtr<ID3D11Buffer> vertex_buffer_ = nullptr;
    ComPtr<ID3D11Buffer> constant_buffer_ = nullptr;

    DirectX::XMMATRIX world;

    static RenderVertices make(const ComPtr<ID3D11Device>& device
        , const std::span<const Vector3f>& vertices);

    void render(ID3D11DeviceContext& device_context
        , const DirectX::XMMATRIX& view_transposed
        , const DirectX::XMMATRIX& projection_transposed) const;
};
