#pragma once
#pragma warning(push)
// macro redefinition
#pragma warning(disable : 4005)
#include <d3d11.h>
#pragma warning(pop)
#include <DirectXMath.h>
#include <Directxcollision.h>

#include <vector>
#include <span>

// Inspired by:
// https://github.com/GPUOpen-LibrariesAndSDKs/GPUParticles11/blob/master/amd_sdk/src/LineRender.h

struct RenderLines
{
    struct LineVertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 color;
    };

    struct LineVSConstantBuffer
    {
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;
    };

    std::vector<LineVertex> vertices_;

    // Owning.
    ID3D11Device* device_ = nullptr;
    ID3D11VertexShader* vertex_shader_ = nullptr;
    ID3D11InputLayout* vertex_layout_ = nullptr;
    ID3D11PixelShader* pixel_shader_ = nullptr;
    ID3D11Buffer* vertex_buffer_ = nullptr;
    ID3D11Buffer* constant_buffer_ = nullptr;

    static RenderLines make(ID3D11Device& device);

    void add_line(const DirectX::XMFLOAT3& p0
        , const DirectX::XMFLOAT3& p1
        , const DirectX::XMFLOAT3& color = DirectX::XMFLOAT3(1.f, 1.f, 1.f));
    void add_lines(const std::span<const DirectX::XMFLOAT3>& points
        , const DirectX::XMFLOAT3& color = DirectX::XMFLOAT3(1.f, 1.f, 1.f));
    void add_bbox(const DirectX::BoundingBox& box
        , const DirectX::XMFLOAT3& color = DirectX::XMFLOAT3(1.f, 1.f, 1.f));

    // #TODO: clean-up signature.
    void render(ID3D11DeviceContext& device_context
        , ID3D11RenderTargetView& render_target_view
        , const D3D11_VIEWPORT& vp
        , const LineVSConstantBuffer& constants);

    RenderLines() = default;
    ~RenderLines();
    RenderLines(RenderLines&& rhs) noexcept;
    RenderLines& operator=(RenderLines&& rhs) = delete;
    RenderLines(const RenderLines& rhs) = delete;
    RenderLines& operator=(const RenderLines&& rhs) = delete;
};
