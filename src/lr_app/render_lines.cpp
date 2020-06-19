#include "render_lines.h"

#include "utils.h"

#include "shaders/ps_lines.h"
#include "shaders/vs_lines.h"

const D3D11_INPUT_ELEMENT_DESC c_lines_vs_layout[] =
{
    {
        .SemanticName = "position",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(RenderLines::LineVertex, position),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
    {
        .SemanticName = "color",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(RenderLines::LineVertex, color),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
};

/*static*/ RenderLines RenderLines::make(ID3D11Device& device)
{
    RenderLines render{};
    render.device_ = &device;
    render.device_->AddRef();

    // VS & IA.
    HRESULT hr = device.CreateVertexShader(
        k_vs_lines
        , sizeof(k_vs_lines)
        , nullptr
        , &render.vertex_shader_);
    Panic(SUCCEEDED(hr));

    hr = device.CreateInputLayout(
        c_lines_vs_layout
        , _countof(c_lines_vs_layout)
        , k_vs_lines
        , sizeof(k_vs_lines)
        , &render.vertex_layout_);
    Panic(SUCCEEDED(hr));

    // PS.
    hr = device.CreatePixelShader(
        k_ps_lines
        , sizeof(k_ps_lines)
        , nullptr, &render.pixel_shader_);
    Panic(SUCCEEDED(hr));

    D3D11_BUFFER_DESC desc{};
#if (0) // Will be created on resize.
    // Create vertex buffer.
    desc.ByteWidth = render.vertices_ * sizeof(LineVertex);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = device.CreateBuffer(&desc, 0, &render.vertex_buffer_);
    Panic(SUCCEEDED(hr));
#endif

    // Create constant buffer.
    desc.ByteWidth = sizeof(LineVSConstantBuffer);
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    device.CreateBuffer(&desc, 0, &render.constant_buffer_);
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
    for (int i = 0; i < std::size(indices); i++)
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
        if (vertex_buffer_)
        {
            vertex_buffer_->Release();
            vertex_buffer_ = nullptr;
        }
        D3D11_BUFFER_DESC desc{};
        // Create vertex buffer.
        // #TODO: trap memory size is less then max UINT.
        desc.ByteWidth      = UINT(vertices_.capacity() * sizeof(LineVertex));
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        HRESULT hr = device_->CreateBuffer(&desc, 0, &vertex_buffer_);
        Panic(SUCCEEDED(hr));
    }
}

void RenderLines::render(ID3D11DeviceContext& device_context
    , ID3D11RenderTargetView& render_target_view
    , const D3D11_VIEWPORT& vp
    , const LineVSConstantBuffer& constants)
{
    if (vertices_.empty())
    {
        return;
    }
    Panic(device_);

    // Copy the CPU buffer into the GPU one.
    D3D11_MAPPED_SUBRESOURCE data;
    HRESULT hr = device_context.Map(vertex_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &data);
    Panic(SUCCEEDED(hr));
    memcpy(data.pData, vertices_.data(), sizeof(LineVertex) * vertices_.size());
    device_context.Unmap(vertex_buffer_, 0);

    // Input Assembler.
    UINT stride = sizeof(LineVertex);
    UINT offset = 0;
    device_context.IASetVertexBuffers(0, 1, &vertex_buffer_, &stride, &offset);
    device_context.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    device_context.IASetInputLayout(vertex_layout_);

    // Vertex Shader.
    device_context.VSSetShader(vertex_shader_, 0, 0);
    device_context.UpdateSubresource(constant_buffer_, 0, nullptr, &constants, 0, 0);
    device_context.VSSetConstantBuffers(0, 1, &constant_buffer_);

    // Rasterizer Stage.
    device_context.RSSetViewports(1, &vp);

    // Pixel Shader.
    device_context.PSSetShader(pixel_shader_, 0, 0);

    // Output Merger.
    ID3D11RenderTargetView* target_view = &render_target_view;
    device_context.OMSetRenderTargets(1, &target_view, nullptr);

    // Draw.
    device_context.Draw(UINT(vertices_.size()), 0);
}

RenderLines::~RenderLines()
{
    if (!device_)
    {
        return;
    }
    device_->Release();
    vertex_shader_->Release();
    vertex_layout_->Release();
    pixel_shader_->Release();
    vertex_buffer_->Release();
    constant_buffer_->Release();
    device_ = nullptr;
    vertex_shader_ = nullptr;
    vertex_layout_ = nullptr;
    pixel_shader_ = nullptr;
    vertex_buffer_ = nullptr;
    constant_buffer_ = nullptr;
}

RenderLines::RenderLines(RenderLines&& rhs) noexcept
    : device_(rhs.device_)
    , vertex_shader_(rhs.vertex_shader_)
    , vertex_layout_(rhs.vertex_layout_)
    , pixel_shader_(rhs.pixel_shader_)
    , vertex_buffer_(rhs.vertex_buffer_)
    , constant_buffer_(rhs.constant_buffer_)
{
    rhs.device_ = nullptr;
    rhs.vertex_shader_ = nullptr;
    rhs.vertex_layout_ = nullptr;
    rhs.pixel_shader_ = nullptr;
    rhs.vertex_buffer_ = nullptr;
    rhs.constant_buffer_ = nullptr;
}
