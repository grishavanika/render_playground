#pragma once
#include "dx_api.h"

#include <vector>
#include <span>

#include "utils.h"
#include "shaders_compiler.h"

// Mostly from:
// https://github.com/GPUOpen-LibrariesAndSDKs/GPUParticles11/blob/master/amd_sdk/src/LineRender.h

struct RenderLines
{
    struct LineVertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 color;
    };

    std::vector<LineVertex> vertices_;

    ComPtr<ID3D11Device> device_ = nullptr;
    VSShader* vs_shader_ = nullptr;
    PSShader* ps_shader_ = nullptr;
    ComPtr<ID3D11Buffer> vertex_buffer_ = nullptr;
    ComPtr<ID3D11Buffer> constant_buffer_ = nullptr;

    DirectX::XMMATRIX world;

    static RenderLines make(const ComPtr<ID3D11Device>& device);

    void add_line(const DirectX::XMFLOAT3& p0
        , const DirectX::XMFLOAT3& p1
        , const DirectX::XMFLOAT3& color = DirectX::XMFLOAT3(1.f, 1.f, 1.f));
    void add_lines(const std::span<const DirectX::XMFLOAT3>& points
        , const DirectX::XMFLOAT3& color = DirectX::XMFLOAT3(1.f, 1.f, 1.f));
    void add_bbox(const DirectX::BoundingBox& box
        , const DirectX::XMFLOAT3& color = DirectX::XMFLOAT3(1.f, 1.f, 1.f));

    void render(ID3D11DeviceContext& device_context
        , const DirectX::XMMATRIX& view_transposed
        , const DirectX::XMMATRIX& projection_transposed) const;
};
