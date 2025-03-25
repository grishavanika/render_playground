#include "render_vertices_only.h"
#include "shaders_compiler.h"
#include "utils.h"

#include <glm/mat4x4.hpp>

struct VerticesVSConstantBuffer
{
    glm::mat4x4 world;
    glm::mat4x4 view;
    glm::mat4x4 projection;
};

/*static*/ RenderVertices RenderVertices::make(const ComPtr<ID3D11Device>& device
    , const std::span<const glm::vec3>& vertices)
{
    RenderVertices render{};
    render.device_ = device;
    render.vertices_.assign(vertices.begin(), vertices.end());
    render.world = glm::mat4x4(1.f);
    D3D11_BUFFER_DESC desc{};
    // Create vertex buffer.
    desc.ByteWidth = UINT(render.vertices_.size() * sizeof(glm::vec3));
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
    , const glm::mat4x4& view
    , const glm::mat4x4& projection) const
{
    if (vertices_.empty())
    {
        return;
    }
    Panic(device_);
    PanicShadersValid(vs_shader_, ps_shader_);

    // Input Assembler.
    UINT stride = sizeof(glm::vec3);
    UINT offset = 0;
    device_context.IASetVertexBuffers(0, 1, vertex_buffer_.GetAddressOf(), &stride, &offset);
    device_context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device_context.IASetInputLayout(vs_shader_->vs_layout.Get());

    // Vertex Shader.
    VerticesVSConstantBuffer vs_constants;
    vs_constants.world = world;
    vs_constants.projection = projection;
    vs_constants.view = view;
    device_context.VSSetShader(vs_shader_->vs.Get(), 0, 0);
    device_context.UpdateSubresource(constant_buffer_.Get(), 0, nullptr, &vs_constants, 0, 0);
    device_context.VSSetConstantBuffers(0, 1, constant_buffer_.GetAddressOf());

    // Pixel Shader.
    device_context.PSSetShader(ps_shader_->ps.Get(), 0, 0);

    // Draw.
    device_context.Draw(UINT(vertices_.size()), 0);
}
