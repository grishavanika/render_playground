#define NOMINMAX
#include <windows.h>

#include <Windows.h>
#include <Windowsx.h>
#include <tchar.h>

#pragma warning(push)
// macro redefinition
#pragma warning(disable : 4005)
#include <d3d11.h>
#pragma warning(pop)

#include <directxmath.h>
using namespace DirectX;

#include <cstdlib>

#include <vector>
#include <unordered_set>

#include "stub_window.h"
#include "utils.h"

#include "model.h"

#include "shaders/vs_basic_phong_lighting.h"
#include "shaders/ps_basic_phong_lighting.h"

#include "render_lines.h"

#define XX_OBJECT_ROTATE() 0
#define XX_WIREFRAME() 0

const D3D11_INPUT_ELEMENT_DESC layout[] =
{
    {
        .SemanticName = "position",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(Vertex, position),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
    {
        .SemanticName = "normal",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(Vertex, normal),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
    {
        .SemanticName = "texcoord",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(Vertex, texture_coord),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
};

#pragma warning(push)
// structure was padded due to alignment specifier
#pragma warning(disable:4324)
struct GameState
{
    float fov_y_ = DegreesToRadians(45.f);
    float aspect_ratio_ = 0.f;
    float mouse_scroll_sensitivity_ = 0.05f;
    float camera_yaw_degrees_ = 90.f;
    float camera_pitch_degrees_ = 0.f;
    float camera_rotation_mouse_sensitivity_ = 0.04f;
    float camera_move_speed_ = 0.5f;

    std::unordered_set<WPARAM> keys_down_;

    XMVECTOR camera_position_     = XMVectorSet(0.0f, 0.0f, -15.0f, 0.0f);
    const XMVECTOR camera_up_dir_ = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR camera_front_dir_    = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMVECTOR camera_right_dir_    = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    D3D11_VIEWPORT vp_{};
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> device_context_;
    ComPtr<IDXGISwapChain> swap_chain_;
    ComPtr<ID3D11RenderTargetView> render_target_view_;
    ComPtr<ID3D11DepthStencilView> depth_buffer_;
};
#pragma warning(pop)

constexpr DXGI_FORMAT GetIndexBufferFormat()
{
    if constexpr ((sizeof(Index) == 2) && std::is_unsigned_v<Index>)
    {
        return DXGI_FORMAT_R16_UINT;
    }
    else if constexpr ((sizeof(Index) == 4) && std::is_unsigned_v<Index>)
    {
        return DXGI_FORMAT_R32_UINT;
    }
    // Intentionally no return to fail compilation.
    // (Control reaching the end of a constexpr function).
}

struct RenderMesh
{
    ComPtr<ID3D11Buffer> vertex_buffer;
    ComPtr<ID3D11Buffer> index_buffer;
    UINT indices_count;
    std::uint32_t ps_texture0_id;

    static RenderMesh make(ID3D11Device& device, const Mesh& mesh)
    {
        RenderMesh render{};
        render.indices_count = UINT(mesh.indices.size());
        render.ps_texture0_id = mesh.texture_diffuse_id;

        D3D11_BUFFER_DESC bd{};

        // VB.
        Panic(!mesh.vertices.empty());
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = UINT(mesh.vertices.size() * sizeof(Vertex));
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA InitData{};
        InitData.pSysMem = mesh.vertices.data();
        HRESULT hr = device.CreateBuffer(&bd, &InitData, &render.vertex_buffer);
        Panic(SUCCEEDED(hr));

        // IB.
        Panic(!mesh.indices.empty());
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = UINT(mesh.indices.size() * sizeof(Index));
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bd.CPUAccessFlags = 0;
        InitData.pSysMem = mesh.indices.data();
        hr = device.CreateBuffer(&bd, &InitData, &render.index_buffer);
        Panic(SUCCEEDED(hr));

        return render;
    }
};

struct RenderTexture
{
    ComPtr<ID3D11ShaderResourceView> texture_view;
    std::uint32_t texture_id;

    static RenderTexture make(ID3D11Device& device, const Texture& texture)
    {
        RenderTexture render{};
        render.texture_id = texture.id;

        D3D11_TEXTURE2D_DESC t2d_desc{};
        t2d_desc.Width = texture.width;
        t2d_desc.Height = texture.height;
        t2d_desc.MipLevels = 1;
        t2d_desc.ArraySize = 1;
        t2d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        t2d_desc.SampleDesc.Count = 1;
        t2d_desc.SampleDesc.Quality = 0;
        t2d_desc.Usage = D3D11_USAGE_DEFAULT;
        t2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        t2d_desc.CPUAccessFlags = 0;
        t2d_desc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA subresource{};
        subresource.pSysMem = texture.data.data();
        subresource.SysMemPitch = (texture.width * c_texture_channels);
        subresource.SysMemSlicePitch = 0; // not used for 2d textures.

        ComPtr<ID3D11Texture2D> texture2d;
        HRESULT hr = device.CreateTexture2D(&t2d_desc, &subresource, &texture2d);
        Panic(SUCCEEDED(hr));

        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
        SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = 1;

        hr = device.CreateShaderResourceView(texture2d.Get(), &SRVDesc, &render.texture_view);
        Panic(SUCCEEDED(hr));

        return render;
    }
};

#pragma warning(push)
// structure was padded due to alignment specifier
#pragma warning(disable:4324)
struct RenderModel
{
    // vs_basic_phong_lighting.hlsl
    struct VSConstantBuffer0
    {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
    };

    // ps_basic_phong_lighting.hlsl
    struct PSConstantBuffer0
    {
        XMVECTOR light_color;
        XMVECTOR light_position;
        XMVECTOR viewer_position;
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
    XMMATRIX world;

    // Tweak light.
    XMVECTOR light_color;
    XMVECTOR light_position;
    XMVECTOR viewer_position;

    static RenderModel make(ID3D11Device& device, const Model& model)
    {
        RenderModel render{};
        render.model = &model;

        // VS & IA.
        HRESULT hr = device.CreateInputLayout(
            layout
            , _countof(layout)
            , k_vs_basic_phong_lighting
            , sizeof(k_vs_basic_phong_lighting)
            , &render.vertex_layout_);
        Panic(SUCCEEDED(hr));

        hr = device.CreateVertexShader(
            k_vs_basic_phong_lighting
            , sizeof(k_vs_basic_phong_lighting)
            , nullptr
            , &render.vertex_shader_);
        Panic(SUCCEEDED(hr));

        // Create the constant buffer for VS.
        D3D11_BUFFER_DESC vs_bd{};
        vs_bd.Usage = D3D11_USAGE_DEFAULT;
        vs_bd.ByteWidth = sizeof(VSConstantBuffer0);
        vs_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        vs_bd.CPUAccessFlags = 0;
        hr = device.CreateBuffer(&vs_bd, nullptr, &render.vs_constant_buffer0_);
        Panic(SUCCEEDED(hr));

        // PS.
        hr = device.CreatePixelShader(
            k_ps_basic_phong_lighting
            , sizeof(k_ps_basic_phong_lighting)
            , nullptr
            , &render.pixel_shader_);
        Panic(SUCCEEDED(hr));

        D3D11_BUFFER_DESC ps_bd{};
        ps_bd.Usage = D3D11_USAGE_DEFAULT;
        ps_bd.ByteWidth = sizeof(PSConstantBuffer0);
        ps_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        ps_bd.CPUAccessFlags = 0;
        hr = device.CreateBuffer(&ps_bd, nullptr, &render.ps_constant_buffer0_);
        Panic(SUCCEEDED(hr));

        // Create the sample state.
        // Texture sampling for PS.
        D3D11_SAMPLER_DESC sampler_desc{};
        sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampler_desc.MinLOD = 0;
        sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
        hr = device.CreateSamplerState(&sampler_desc, &render.sampler_linear_);
        Panic(SUCCEEDED(hr));

        for (std::uint32_t i = 0; i < model.meshes_count_; ++i)
        {
            render.meshes.push_back(RenderMesh::make(
                device, model.get_mesh(i)));
        }
        for (std::uint32_t i = 0; i < model.textures_count_; ++i)
        {
            render.textures.push_back(RenderTexture::make(
                device, model.get_texture(i)));
        }
        return render;
    }

    void render(ID3D11DeviceContext& device_context
        , const DirectX::XMMATRIX& view_transposed
        , const DirectX::XMMATRIX& projection_transposed) const
    {
        // Parameters for VS.
        VSConstantBuffer0 vs_cb0;
        vs_cb0.world      = XMMatrixTranspose(world);
        vs_cb0.view       = view_transposed;
        vs_cb0.projection = projection_transposed;

        // Parameters for PS.
        PSConstantBuffer0 ps_cb0;
        ps_cb0.light_color     = light_color;
        ps_cb0.viewer_position = viewer_position;
        ps_cb0.light_position  = light_position;

        for (const RenderMesh& render_mesh : meshes)
        {
            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            // Input Assembler.
            device_context.IASetVertexBuffers(0, 1, render_mesh.vertex_buffer.GetAddressOf(), &stride, &offset);
            device_context.IASetIndexBuffer(render_mesh.index_buffer.Get(), GetIndexBufferFormat(), 0);
            device_context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            device_context.IASetInputLayout(vertex_layout_.Get());
            // Vertex Shader.
            device_context.VSSetShader(vertex_shader_.Get(), nullptr, 0);
            device_context.UpdateSubresource(vs_constant_buffer0_.Get(), 0, nullptr, &vs_cb0, 0, 0);
            device_context.VSSetConstantBuffers(0, 1, vs_constant_buffer0_.GetAddressOf());
            // Pixel Shader.
            device_context.PSSetShader(pixel_shader_.Get(), nullptr, 0);
            device_context.UpdateSubresource(ps_constant_buffer0_.Get(), 0, nullptr, &ps_cb0, 0, 0);
            device_context.PSSetConstantBuffers(0, 1, ps_constant_buffer0_.GetAddressOf());
            if (ID3D11ShaderResourceView* ps_texture0 = get_texture(render_mesh.ps_texture0_id))
            {
                device_context.PSSetShaderResources(0, 1, &ps_texture0);
                device_context.PSSetSamplers(0, 1, sampler_linear_.GetAddressOf());
            }

            // Actual draw call.
            device_context.DrawIndexed(render_mesh.indices_count, 0, 0);
        }
    }

    ID3D11ShaderResourceView* get_texture(std::uint32_t id) const
    {
        for (const RenderTexture& texture : textures)
        {
            if (texture.texture_id == id)
            {
                Panic(texture.texture_view);
                return texture.texture_view.Get();
            }
        }
        return nullptr;
    }
};
#pragma warning(pop)

int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
#if (XX_HAS_TEXTURE_COORDS() && XX_HAS_NORMALS())
    const char* const obj = R"(K:\backpack\backpack.lr.bin)";
#elif (XX_HAS_NORMALS())
    const char* const obj = R"(K:\skull\skull.lr.bin)";
#else
    const char* const obj = R"(K:\tyrannosaurus-rex-skeleton\source\dyno_tex.lr.bin)";
#endif

    Model model = LoadModel(obj);

    GameState game;
    StubWindow window("xxx_lr");
    window.on_message(WM_PAINT
        , [](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        PAINTSTRUCT ps;
        (void)::BeginPaint(hwnd, &ps);
        (void)::EndPaint(hwnd, &ps);
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
    window.on_message(WM_DESTROY
        , [](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        ::PostQuitMessage(0);
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
    window.on_message(WM_SIZE
        , [&game](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        if (!game.device_)
        {
            // Window initialization; ignore.
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        }
        if (wparam == SIZE_MINIMIZED)
        {
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        }
        // Handling Window Resizing:
        // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi?redirectedfrom=MSDN#handling-window-resizing
        Panic(game.device_);
        Panic(game.swap_chain_);
        Panic(game.device_context_);
        Panic(game.render_target_view_);
        Panic(game.depth_buffer_);

        const float width = LOWORD(lparam);
        const float height = HIWORD(lparam);
        game.aspect_ratio_ = (width / static_cast<FLOAT>(height));
        game.device_context_->OMSetRenderTargets(0, 0, 0);
        game.render_target_view_.Reset();
        // Preserve the existing buffer count and format.
        HRESULT hr = game.swap_chain_->ResizeBuffers(0, UINT(width), UINT(height), DXGI_FORMAT_UNKNOWN, 0);
        Panic(SUCCEEDED(hr));

        // Get buffer and create a render-target-view.
        ComPtr<ID3D11Texture2D> buffer;
        hr = game.swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &buffer);
        Panic(SUCCEEDED(hr));

        hr = game.device_->CreateRenderTargetView(buffer.Get() , nullptr, &game.render_target_view_);
        Panic(SUCCEEDED(hr));
        buffer.Reset();

        game.depth_buffer_.Reset();

        D3D11_TEXTURE2D_DESC depthTextureDesc{};
        depthTextureDesc.Width = UINT(width);
        depthTextureDesc.Height = UINT(height);
        depthTextureDesc.MipLevels = 1;
        depthTextureDesc.ArraySize = 1;
        depthTextureDesc.SampleDesc.Count = 1;
        depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ComPtr<ID3D11Texture2D> depth_stencil_texture;
        hr = game.device_->CreateTexture2D(&depthTextureDesc, nullptr, &depth_stencil_texture);
        Panic(SUCCEEDED(hr));

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = depthTextureDesc.Format;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

        hr = game.device_->CreateDepthStencilView(depth_stencil_texture.Get(), &dsvDesc, &game.depth_buffer_);
        depth_stencil_texture.Reset();
        Panic(SUCCEEDED(hr));

        game.device_context_->OMSetRenderTargets(1, game.render_target_view_.GetAddressOf(), game.depth_buffer_.Get());

        // Set up the viewport.
        game.vp_.Width = static_cast<FLOAT>(width);
        game.vp_.Height = static_cast<FLOAT>(height);

        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
    window.on_message(WM_MOUSEWHEEL
        , [&game](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        const float delta_wheel = GET_WHEEL_DELTA_WPARAM(wparam);
        const float dv = (delta_wheel / WHEEL_DELTA) * game.mouse_scroll_sensitivity_;
        game.fov_y_ = std::clamp(game.fov_y_ - dv, DegreesToRadians(1.f), DegreesToRadians(90.f));
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });

    RAWINPUTDEVICE raw_mouse{};
    raw_mouse.usUsagePage = 0x01; // generic
    raw_mouse.usUsage = 0x02; // mouse
    raw_mouse.dwFlags = RIDEV_INPUTSINK;
    raw_mouse.hwndTarget = window.wnd();
    Panic(::RegisterRawInputDevices(&raw_mouse, 1, sizeof(RAWINPUTDEVICE)));

    window.on_message(WM_INPUT
        , [&game](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        RAWINPUT raw{};
        UINT size = sizeof(raw);
        const UINT status = ::GetRawInputData(HRAWINPUT(lparam)
            , RID_INPUT
            , &raw
            , &size
            , sizeof(RAWINPUTHEADER));
        Panic(status == size);
        Panic(raw.header.dwType == RIM_TYPEMOUSE);
        Panic((raw.data.mouse.usFlags & MOUSE_MOVE_RELATIVE) == MOUSE_MOVE_RELATIVE);
        const LONG x_delta = raw.data.mouse.lLastX;
        const LONG y_delta = raw.data.mouse.lLastY;

        const float d_yaw = (x_delta * game.camera_rotation_mouse_sensitivity_);
        const float d_pitch = (y_delta * game.camera_rotation_mouse_sensitivity_);

        // https://learnopengl.com/Getting-started/Camera
        game.camera_yaw_degrees_ -= d_yaw;
        game.camera_pitch_degrees_ = std::clamp(game.camera_pitch_degrees_ - d_pitch, -89.0f, 89.0f);

        const float yaw = DegreesToRadians(game.camera_yaw_degrees_);
        const float pitch = DegreesToRadians(game.camera_pitch_degrees_);

        const float x = cosf(yaw) * cosf(pitch);
        const float y = sinf(pitch);
        const float z = sinf(yaw) * cosf(DegreesToRadians(game.camera_pitch_degrees_));

        game.camera_front_dir_ = XMVector3Normalize(XMVectorSet(x, y, z, 0.f));
        game.camera_right_dir_ = XMVector3Normalize(XMVector3Cross(game.camera_front_dir_, game.camera_up_dir_));

        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
    window.on_message(WM_KEYUP
        , [&game](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        (void)game.keys_down_.erase(wparam);
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
    window.on_message(WM_KEYDOWN
        , [&game](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        (void)game.keys_down_.insert(wparam);
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });

    Panic(::ShowWindow(window.wnd(), SW_SHOW) == 0/*was previously hidden*/);

    RECT client_rect{};
    Panic(!!::GetClientRect(window.wnd(), &client_rect));
    const UINT client_width = (client_rect.right - client_rect.left);
    const UINT client_height = (client_rect.bottom - client_rect.top);
    game.aspect_ratio_ = (client_width / static_cast<FLOAT>(client_height));
    
    { // Calculate default camera orientation from initial angles.
        const float yaw = DegreesToRadians(game.camera_yaw_degrees_);
        const float pitch = DegreesToRadians(game.camera_pitch_degrees_);

        const float x = cosf(yaw) * cosf(pitch);
        const float y = sinf(pitch);
        const float z = sinf(yaw) * cosf(DegreesToRadians(game.camera_pitch_degrees_));

        game.camera_front_dir_ = XMVector3Normalize(XMVectorSet(x, y, z, 0.f));
        game.camera_right_dir_ = XMVector3Normalize(XMVector3Cross(game.camera_front_dir_, game.camera_up_dir_));
    }

    // Device initialization.
    DXGI_SWAP_CHAIN_DESC sc_desc{};
    sc_desc.BufferCount = 1;
    sc_desc.BufferDesc.Width = client_width;
    sc_desc.BufferDesc.Height = client_height;
    sc_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sc_desc.BufferDesc.RefreshRate.Numerator = 60;
    sc_desc.BufferDesc.RefreshRate.Denominator = 1;
    sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sc_desc.OutputWindow = window.wnd();
    sc_desc.SampleDesc.Count = 1;
    sc_desc.SampleDesc.Quality = 0;
    sc_desc.Windowed = TRUE;

    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
    D3D_FEATURE_LEVEL unused = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = ::D3D11CreateDeviceAndSwapChain(
        nullptr
        , D3D_DRIVER_TYPE_HARDWARE
        , nullptr
        , D3D11_CREATE_DEVICE_DEBUG
        , &feature_level
        , 1
        , D3D11_SDK_VERSION
        , &sc_desc
        , &game.swap_chain_
        , &game.device_
        , &unused
        , &game.device_context_);
    Panic(SUCCEEDED(hr));

    // Create a render target view.
    ComPtr<ID3D11Texture2D> back_buffer;
    hr = game.swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &back_buffer);
    Panic(SUCCEEDED(hr));
    hr = game.device_->CreateRenderTargetView(back_buffer.Get(), nullptr, &game.render_target_view_);
    Panic(SUCCEEDED(hr));
    back_buffer.Reset();

    // Setup the viewport.
    game.vp_.Width = static_cast<FLOAT>(client_width);
    game.vp_.Height = static_cast<FLOAT>(client_height);
    game.vp_.MinDepth = 0.0f;
    game.vp_.MaxDepth = 1.0f;
    game.vp_.TopLeftX = 0;
    game.vp_.TopLeftY = 0;

    // Z-test/buffer.
    D3D11_TEXTURE2D_DESC dept_texture_desc{};
    dept_texture_desc.Width = client_width;
    dept_texture_desc.Height = client_height;
    dept_texture_desc.MipLevels = 1;
    dept_texture_desc.ArraySize = 1;
    dept_texture_desc.SampleDesc.Count = 1;
    dept_texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dept_texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ComPtr<ID3D11Texture2D> depth_stencil_texture;
    hr = game.device_->CreateTexture2D(&dept_texture_desc, nullptr, &depth_stencil_texture);
    Panic(SUCCEEDED(hr));

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = dept_texture_desc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

    hr = game.device_->CreateDepthStencilView(depth_stencil_texture.Get(), &dsvDesc, &game.depth_buffer_);
    depth_stencil_texture.Reset();
    Panic(SUCCEEDED(hr));

    // Ability to enable/disable wireframe.
    D3D11_RASTERIZER_DESC wfd{};
#if (XX_WIREFRAME())
    wfd.FillMode = D3D11_FILL_WIREFRAME;
#else
    wfd.FillMode = D3D11_FILL_SOLID;
#endif
    wfd.CullMode = D3D11_CULL_NONE;
    wfd.DepthClipEnable = TRUE;
    wfd.AntialiasedLineEnable = TRUE;
    ComPtr<ID3D11RasterizerState> rasterizer_state;
    hr = game.device_->CreateRasterizerState(&wfd, &rasterizer_state);
    Panic(SUCCEEDED(hr));

    RenderModel render_model = RenderModel::make(*game.device_.Get(), model);
    RenderLines render_lines = RenderLines::make(game.device_);
    render_lines.add_bbox(BoundingBox(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(1.f, 1.f, 1.f)));
    // Positive World X direction. RED.
    render_lines.add_line(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(1.f, 0.f, 0.f), XMFLOAT3(1.f, 0.f, 0.f));
    // Positive World Y direction. GREEN.
    render_lines.add_line(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT3(0.f, 1.f, 0.f));
    // Positive World Z direction. BLUE.
    render_lines.add_line(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(0.f, 0.f, 1.f));

    // Initialize the view matrix.
    XMMATRIX projection = XMMatrixIdentity();
    XMMATRIX view = XMMatrixIdentity();

    const DWORD start_time = ::GetTickCount();

    // Main message loop
    MSG msg{};
    while (WM_QUIT != msg.message)
    {
        if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        if (game.keys_down_.contains(0x57)) // W
        {
            game.camera_position_ += (game.camera_move_speed_ * game.camera_front_dir_);
        }
        if (game.keys_down_.contains(0x53)) // S
        {
            game.camera_position_ -= (game.camera_front_dir_ * game.camera_move_speed_);
        }
        if (game.keys_down_.contains(0x41)) // A
        {
            game.camera_position_ += (game.camera_right_dir_ * game.camera_move_speed_);
        }
        if (game.keys_down_.contains(0x44)) // D
        {
            game.camera_position_ -= (game.camera_right_dir_ * game.camera_move_speed_);
        }
        if (game.keys_down_.contains(0x51)) // Q
        {
            game.camera_position_ -= (game.camera_up_dir_ * game.camera_move_speed_);
        }
        if (game.keys_down_.contains(0x45)) // E
        {
            game.camera_position_ += (game.camera_up_dir_ * game.camera_move_speed_);
        }

        projection = XMMatrixPerspectiveFovLH(game.fov_y_
            , game.aspect_ratio_
            , 0.01f    // NearZ
            , 100.0f); // FarZ
        const float t = (::GetTickCount() - start_time) / 1000.0f;

        view = XMMatrixLookAtLH(game.camera_position_
            , game.camera_position_ + game.camera_front_dir_
            , game.camera_up_dir_);

        const float scale = 2.f;
        render_model.world = XMMatrixIdentity()
            * XMMatrixScaling(scale, scale, scale)
            * XMMatrixRotationY(2.5);
#if (XX_OBJECT_ROTATE())
#if (XX_HAS_TEXTURE_COORDS())
        render_model.world *= XMMatrixRotationX(t);
#else
        render_model.world *= XMMatrixRotationY(t);
#endif
#endif
        render_model.world *= XMMatrixTranslation(-2.f, 0.f, 0.f);

        render_model.light_color = 20 * XMVectorSet(1.f, 1.f, 1.f, 1.0);
        render_model.viewer_position = game.camera_position_;
#if (1)
        // render_model.light_position = XMVectorSet(0.0f, 0.0f, -15.0f, 0.0f);
        render_model.light_position = game.camera_position_;
#else
        const float radius = 20.0f;
        const float cam_x = (sinf(t) * radius);
        const float cam_z = (cosf(t) * radius);
        render_model.light_position = XMVectorSet(cam_x, 0.0f, cam_z, 0.0f);
#endif

        // Clear.
        const float c_clear_color[4] = {0.f, 0.f, 0.0f, 1.0f};
        game.device_context_->ClearRenderTargetView(game.render_target_view_.Get(), c_clear_color);
        game.device_context_->ClearDepthStencilView(game.depth_buffer_.Get()
            , D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        // Output Merger.
        game.device_context_->OMSetRenderTargets(1, game.render_target_view_.GetAddressOf(), game.depth_buffer_.Get());
        // Rasterizer Stage.
        game.device_context_->RSSetState(rasterizer_state.Get());
        game.device_context_->RSSetViewports(1, &game.vp_);

        render_lines.render(*game.device_context_.Get()
            , XMMatrixTranspose(view)
            , XMMatrixTranspose(projection));
        render_model.render(*game.device_context_.Get()
            , XMMatrixTranspose(view)
            , XMMatrixTranspose(projection));

        // Present.
        const UINT SyncInterval = 1; // Synchronize presentation after single vertical blank.
        hr = game.swap_chain_->Present(SyncInterval, 0);
        Panic(SUCCEEDED(hr));
    }

    return 0;
}
