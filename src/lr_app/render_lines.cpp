#include "render_lines.h"
#include "shaders_compiler.h"
#include "utils.h"

struct LineVSConstantBuffer
{
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
};

/*static*/ RenderLines RenderLines::make(const ComPtr<ID3D11Device>& device
    , const ShadersRef& shaders)
{
    RenderLines render{};
    render.device_ = device;

    shaders.compiler->create_vs(*device.Get()
        , *shaders.vs_shader
        , render.vertex_shader_
        , render.vertex_layout_);
    shaders.compiler->create_ps(*device.Get()
        , *shaders.ps_shader
        , render.pixel_shader_);

    D3D11_BUFFER_DESC desc{};
#if (0) // Will be created on resize.
    // Create vertex buffer.
    desc.ByteWidth = render.vertices_ * sizeof(LineVertex);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = device->CreateBuffer(&desc, 0, &render.vertex_buffer_);
    Panic(SUCCEEDED(hr));
#endif

    // Create constant buffer.
    desc.ByteWidth = sizeof(LineVSConstantBuffer);
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    HRESULT hr = device->CreateBuffer(&desc, 0, &render.constant_buffer_);
    Panic(SUCCEEDED(hr));

    return render;
}

void RenderLines::add_line(const DirectX::XMFLOAT3& p0
    , const DirectX::XMFLOAT3& p1
    , const DirectX::XMFLOAT3& color /*= DirectX::XMFLOAT3(1.f, 1.f, 1.f)*/)
{
    const DirectX::XMFLOAT3 points[2] = {p0, p1};
    add_lines(points, color);
}

void RenderLines::add_bbox(const DirectX::BoundingBox& box
    , const DirectX::XMFLOAT3& color /*= DirectX::XMFLOAT3(1.f, 1.f, 1.f)*/)
{
    DirectX::XMFLOAT3 corners[8];
    box.GetCorners(corners);

    const int indices[24] =
    {
        0, 1, 1, 2, 2, 3, 3, 0,
        3, 7, 7, 6, 6, 2, 7, 4,
        4, 0, 4, 5, 5, 1, 5, 6
    };

    DirectX::XMFLOAT3 points[std::size(indices)];
    for (std::size_t i = 0; i < std::size(indices); ++i)
    {
        points[i] = corners[indices[i]];
    }

    add_lines(points, color);
}

void RenderLines::add_lines(const std::span<const DirectX::XMFLOAT3>& points
    , const DirectX::XMFLOAT3& color /*= DirectX::XMFLOAT3(1.f, 1.f, 1.f)*/)
{
    Panic(!points.empty());
    Panic((points.size() % 2) == 0);

    const std::size_t old_capacity = vertices_.capacity();
    vertices_.reserve(vertices_.size() + points.size());
    for (const DirectX::XMFLOAT3& p : points)
    {
        LineVertex& v = vertices_.emplace_back(LineVertex{});
        v.position = p;
        v.color = color;
    }

    if (vertices_.capacity() > old_capacity)
    {
        Panic(device_);
        vertex_buffer_.Reset();
        D3D11_BUFFER_DESC desc{};
        // Create vertex buffer.
        desc.ByteWidth      = UINT(vertices_.capacity() * sizeof(LineVertex));
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        HRESULT hr = device_->CreateBuffer(&desc, 0, &vertex_buffer_);
        Panic(SUCCEEDED(hr));
    }
}

void RenderLines::render(ID3D11DeviceContext& device_context
    , const DirectX::XMMATRIX& view_transposed
    , const DirectX::XMMATRIX& projection_transposed) const
{
    if (vertices_.empty())
    {
        return;
    }
    Panic(device_);

    // Copy the CPU buffer into the GPU one.
    D3D11_MAPPED_SUBRESOURCE data;
    HRESULT hr = device_context.Map(vertex_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &data);
    Panic(SUCCEEDED(hr));
    memcpy(data.pData, vertices_.data(), sizeof(LineVertex) * vertices_.size());
    device_context.Unmap(vertex_buffer_.Get(), 0);

    // Input Assembler.
    UINT stride = sizeof(LineVertex);
    UINT offset = 0;
    device_context.IASetVertexBuffers(0, 1, vertex_buffer_.GetAddressOf(), &stride, &offset);
    device_context.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    device_context.IASetInputLayout(vertex_layout_.Get());

    // Vertex Shader.
    LineVSConstantBuffer vs_constants;
    vs_constants.projection = projection_transposed;
    vs_constants.view = view_transposed;
    device_context.VSSetShader(vertex_shader_.Get(), 0, 0);
    device_context.UpdateSubresource(constant_buffer_.Get(), 0, nullptr, &vs_constants, 0, 0);
    device_context.VSSetConstantBuffers(0, 1, constant_buffer_.GetAddressOf());

    // Pixel Shader.
    device_context.PSSetShader(pixel_shader_.Get(), 0, 0);

    // Draw.
    device_context.Draw(UINT(vertices_.size()), 0);
}
