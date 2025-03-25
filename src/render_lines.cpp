#include "render_lines.h"
#include "shaders_compiler.h"
#include "utils.h"

#include <glm/mat4x4.hpp>

struct LineVSConstantBuffer
{
    glm::mat4x4 world;
    glm::mat4x4 view;
    glm::mat4x4 projection;
};

/*static*/ RenderLines RenderLines::make(const ComPtr<ID3D11Device>& device)
{
    RenderLines render{};
    render.device_ = device;
    render.world = glm::mat4x4(1.0f);

    D3D11_BUFFER_DESC desc{};
#if (0) // Will be created on resize.
    // Create vertex buffer.
    desc.ByteWidth = UINT(render.vertices_.size() * sizeof(LineVertex));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = device->CreateBuffer(&desc, 0, &render.vertex_buffer_);
    Panic(SUCCEEDED(hr));
#else
    HRESULT hr;
#endif

    // Create constant buffer.
    desc.ByteWidth = sizeof(LineVSConstantBuffer);
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = device->CreateBuffer(&desc, 0, &render.constant_buffer_);
    Panic(SUCCEEDED(hr));

    return render;
}

void RenderLines::add_line(const glm::vec3& p0, const glm::vec3& p1
    , const glm::vec3& color /*= glm::vec3(1.f)*/)
{
    const glm::vec3 points[2] = {p0, p1};
    add_lines(points, color);
}

void RenderLines::add_bb(const glm::vec3& min, const glm::vec3& max
    , const glm::vec3& color /*= glm::vec3(1.f)*/)
{
    glm::vec3 corners[8];
    corners[0].x = min.x;
    corners[0].y = min.y;
    corners[0].z = min.z;
    corners[1].x = max.x;
    corners[1].y = min.y;
    corners[1].z = min.z;
    corners[2].x = max.x;
    corners[2].y = max.y;
    corners[2].z = min.z;
    corners[3].x = min.x;
    corners[3].y = max.y;
    corners[3].z = min.z;
    corners[4].x = min.x;
    corners[4].y = min.y;
    corners[4].z = max.z;
    corners[5].x = min.x;
    corners[5].y = max.y;
    corners[5].z = max.z;
    corners[6].x = max.x;
    corners[6].y = max.y;
    corners[6].z = max.z;
    corners[7].x = max.x;
    corners[7].y = min.y;
    corners[7].z = max.z;

    const int indices[] =
    {
        0, 1, 1, 2, 2, 3, 3, 0,
        3, 5, 5, 4, 4, 0, 4, 7,
        7, 6, 6, 5, 6, 2, 7, 1
    };

    glm::vec3 points[std::size(indices)];
    for (std::size_t i = 0; i < std::size(indices); ++i)
    {
        points[i] = corners[indices[i]];
    }

    add_lines(points, color);
}

void RenderLines::clear()
{
    vertices_.clear();
}

void RenderLines::add_lines(const std::span<const glm::vec3>& points
    , const glm::vec3& color /*= glm::vec3(1.f)*/)
{
    Panic(!points.empty());
    Panic((points.size() % 2) == 0);

    const std::size_t old_capacity = vertices_.capacity();
    vertices_.reserve(vertices_.size() + points.size());
    for (const glm::vec3& p : points)
    {
        vertices_.emplace_back(LineVertex{p, color});
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
    , const glm::mat4x4& view
    , const glm::mat4x4& projection) const
{
    if (vertices_.empty())
    {
        return;
    }
    Panic(device_);
    PanicShadersValid(vs_shader_, ps_shader_);

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
    device_context.IASetInputLayout(vs_shader_->vs_layout.Get());

    // Vertex Shader.
    LineVSConstantBuffer vs_constants;
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
