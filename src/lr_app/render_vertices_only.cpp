#include "render_vertices_only.h"
#include "shaders_compiler.h"
#include "utils.h"

#include <DirectXMath.h>

struct VerticesVSConstantBuffer
{
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
};

/*static*/ RenderVertices RenderVertices::make(const ComPtr<ID3D11Device>& device
    , const std::span<const Vector3f>& vertices)
{
    RenderVertices render{};
    render.device_ = device;
    render.vertices_.assign(vertices.cbegin(), vertices.cend());
    render.world = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
    D3D11_BUFFER_DESC desc{};
    // Create vertex buffer.
    desc.ByteWidth = UINT(render.vertices_.size() * sizeof(Vector3f));
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = render.vertices_.empty() ? nullptr : render.vertices_.data();
    HRESULT hr = device->CreateBuffer(&desc, &data, &render.vertex_buffer_);
    Panic(SUCCEEDED(hr));
    // Create constant buffer.
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.ByteWidth = sizeof(VerticesVSConstantBuffer);
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = device->CreateBuffer(&desc, 0, &render.constant_buffer_);
    Panic(SUCCEEDED(hr));

    return render;
}

void RenderVertices::render(ID3D11DeviceContext& device_context
    , const DirectX::XMMATRIX& view_transposed
    , const DirectX::XMMATRIX& projection_transposed) const
{
    if (vertices_.empty())
    {
        return;
    }
    Panic(device_);

    // Input Assembler.
    UINT stride = sizeof(Vector3f);
    UINT offset = 0;
    device_context.IASetVertexBuffers(0, 1, vertex_buffer_.GetAddressOf(), &stride, &offset);
    device_context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device_context.IASetInputLayout(vertex_layout_.Get());

    // Vertex Shader.
    VerticesVSConstantBuffer vs_constants;
    vs_constants.world = world;
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
