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

#include "shaders/vs.h"
#include "shaders/ps.h"

#include "stub_window.h"
#include "utils.h"

#include "model.h"

#define XX_OBJECT_ROTATE() 1
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
    float camera_yaw_degrees_ = 0.f;
    float camera_pitch_degrees_ = 0.f;
    float camera_rotation_mouse_sensitivity_ = 0.04f;
    float camera_move_speed_ = 0.5f;

    std::unordered_set<WPARAM> keys_down_;

    XMVECTOR camera_position_     = XMVectorSet(0.0f, 0.0f, -15.0f, 0.0f);
    const XMVECTOR camera_up_dir_ = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR camera_front_dir_    = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMVECTOR camera_right_dir_    = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    D3D11_VIEWPORT vp_{};
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* device_context_ = nullptr;
    IDXGISwapChain* swap_chain_ = nullptr;
    ID3D11RenderTargetView* render_target_view_ = nullptr;
    ID3D11DepthStencilView* depth_buffer_ = nullptr;
};
#pragma warning(pop)

// Create needed resources AND leak them for now.
struct RenderMesh
{
    ID3D11Buffer* vertex_buffer;
    ID3D11Buffer* index_buffer;
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
        bd.ByteWidth = UINT(mesh.indices.size() * sizeof(WORD));
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
    ID3D11ShaderResourceView* texture_view;
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

        ID3D11Texture2D* texture2d = nullptr;
        HRESULT hr = device.CreateTexture2D(&t2d_desc, &subresource, &texture2d);
        Panic(SUCCEEDED(hr));

        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
        SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = 1;

        hr = device.CreateShaderResourceView(texture2d, &SRVDesc, &render.texture_view);
        Panic(SUCCEEDED(hr));
        texture2d->Release();

        return render;
    }
};

struct RenderModel
{
    const Model* model;
    std::vector<RenderMesh> meshes;
    std::vector<RenderTexture> textures;

    static RenderModel make(ID3D11Device& device, const Model& model)
    {
        RenderModel render{};
        render.model = &model;

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

    ID3D11ShaderResourceView* get_texture(std::uint32_t id)
    {
        for (const RenderTexture& texture : textures)
        {
            if (texture.texture_id == id)
            {
                Panic(texture.texture_view);
                return texture.texture_view;
            }
        }
        return nullptr;
    }
};

struct ConstantBuffer
{
    XMMATRIX world;
    XMMATRIX view;
    XMMATRIX projection;
};

int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
#if (XX_HAS_TEXTURE_COORDS())
    const char* const obj = R"(K:\backpack\backpack.lr.bin)";
#else
    const char* const obj = R"(K:\skull\skull.lr.bin)";
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
        game.render_target_view_->Release();
        game.render_target_view_ = nullptr;
        // Preserve the existing buffer count and format.
        HRESULT hr = game.swap_chain_->ResizeBuffers(0, UINT(width), UINT(height), DXGI_FORMAT_UNKNOWN, 0);
        Panic(SUCCEEDED(hr));

        // Get buffer and create a render-target-view.
        ID3D11Texture2D* buffer = nullptr;
        hr = game.swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buffer);
        Panic(SUCCEEDED(hr));

        hr = game.device_->CreateRenderTargetView(buffer , nullptr, &game.render_target_view_);
        Panic(SUCCEEDED(hr));
        buffer->Release();
        buffer = nullptr;

        game.depth_buffer_->Release();
        game.depth_buffer_ = nullptr;

        D3D11_TEXTURE2D_DESC depthTextureDesc{};
        depthTextureDesc.Width = UINT(width);
        depthTextureDesc.Height = UINT(height);
        depthTextureDesc.MipLevels = 1;
        depthTextureDesc.ArraySize = 1;
        depthTextureDesc.SampleDesc.Count = 1;
        depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ID3D11Texture2D* depth_stencil_texture = nullptr;
        hr = game.device_->CreateTexture2D(&depthTextureDesc, nullptr, &depth_stencil_texture);
        Panic(SUCCEEDED(hr));

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = depthTextureDesc.Format;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

        hr = game.device_->CreateDepthStencilView(depth_stencil_texture, &dsvDesc, &game.depth_buffer_);
        depth_stencil_texture->Release();
        Panic(SUCCEEDED(hr));

        game.device_context_->OMSetRenderTargets(1, &game.render_target_view_, game.depth_buffer_);

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
    ID3D11Texture2D* back_buffer = nullptr;
    hr = game.swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&back_buffer));
    Panic(SUCCEEDED(hr));
    hr = game.device_->CreateRenderTargetView(back_buffer, nullptr, &game.render_target_view_);
    Panic(SUCCEEDED(hr));
    back_buffer->Release();

    // Setup the viewport.
    game.vp_.Width = static_cast<FLOAT>(client_width);
    game.vp_.Height = static_cast<FLOAT>(client_height);
    game.vp_.MinDepth = 0.0f;
    game.vp_.MaxDepth = 1.0f;
    game.vp_.TopLeftX = 0;
    game.vp_.TopLeftY = 0;

    // VS & IA.
    ID3D11VertexShader* vertex_shader = nullptr;
    hr = game.device_->CreateVertexShader(k_VS, sizeof(k_VS), nullptr, &vertex_shader);
    Panic(SUCCEEDED(hr));

    ID3D11InputLayout* vertex_layout = nullptr;
    hr = game.device_->CreateInputLayout(layout, _countof(layout), k_VS, sizeof(k_VS), &vertex_layout);
    Panic(SUCCEEDED(hr));

    // PS.
    ID3D11PixelShader* pixel_shader = nullptr;
    hr = game.device_->CreatePixelShader(k_PS, sizeof(k_PS), nullptr, &pixel_shader);
    Panic(SUCCEEDED(hr));

    // Create the constant buffer
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    ID3D11Buffer* constant_buffer = nullptr;
    hr = game.device_->CreateBuffer(&bd, nullptr, &constant_buffer);
    Panic(SUCCEEDED(hr));

    // Z-test/buffer.
    D3D11_TEXTURE2D_DESC dept_texture_desc{};
    dept_texture_desc.Width = client_width;
    dept_texture_desc.Height = client_height;
    dept_texture_desc.MipLevels = 1;
    dept_texture_desc.ArraySize = 1;
    dept_texture_desc.SampleDesc.Count = 1;
    dept_texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dept_texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depth_stencil_texture = nullptr;
    hr = game.device_->CreateTexture2D(&dept_texture_desc, nullptr, &depth_stencil_texture);
    Panic(SUCCEEDED(hr));

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = dept_texture_desc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

    hr = game.device_->CreateDepthStencilView(depth_stencil_texture, &dsvDesc, &game.depth_buffer_);
    depth_stencil_texture->Release();
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
    ID3D11RasterizerState* rasterizerState = nullptr;
    hr = game.device_->CreateRasterizerState(&wfd, &rasterizerState);
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
    ID3D11SamplerState* sampler_linear = NULL;
    hr = game.device_->CreateSamplerState(&sampler_desc, &sampler_linear);
    Panic(SUCCEEDED(hr));

    RenderModel render_model = RenderModel::make(*game.device_, model);

    // Initialize the view matrix.
    XMMATRIX projection = XMMatrixIdentity();
    XMMATRIX world = XMMatrixIdentity();
    XMMATRIX view = XMMatrixIdentity();

    const DWORD dwTimeStart = ::GetTickCount();

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
        const float t = (::GetTickCount() - dwTimeStart) / 1000.0f;

        view = XMMatrixLookAtLH(game.camera_position_
            , game.camera_position_ + game.camera_front_dir_
            , game.camera_up_dir_);

        const float scale = 2.f;
        world = XMMatrixScaling(scale, scale, scale)
            * XMMatrixRotationY(2.5);

#if (XX_OBJECT_ROTATE())
#if (XX_HAS_TEXTURE_COORDS())
        world = (world * XMMatrixRotationX(t));
#else
        world = (world * XMMatrixRotationY(t));
#endif
#endif
#if (1) // translate
        world = (world * XMMatrixTranslation(+40.f, -10.f, +5.f));
#endif

        // Clear.
        const float c_clear_color[4] = {1.f, 1.f, 1.0f, 1.0f};
        game.device_context_->ClearRenderTargetView(game.render_target_view_, c_clear_color);
        game.device_context_->ClearDepthStencilView(game.depth_buffer_
            , D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        // Render.
        for (const RenderMesh& render_mesh : render_model.meshes)
        {
            ConstantBuffer cb;
            cb.world      = XMMatrixTranspose(world);
            cb.view       = XMMatrixTranspose(view);
            cb.projection = XMMatrixTranspose(projection);

            ID3D11ShaderResourceView* ps_texture0 = render_model.get_texture(render_mesh.ps_texture0_id);

            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            // Input Assembler.
            game.device_context_->IASetVertexBuffers(0, 1, &render_mesh.vertex_buffer, &stride, &offset);
            game.device_context_->IASetIndexBuffer(render_mesh.index_buffer, DXGI_FORMAT_R16_UINT, 0);
            game.device_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            game.device_context_->IASetInputLayout(vertex_layout);
            // Vertex Shader.
            game.device_context_->VSSetShader(vertex_shader, nullptr, 0);
            game.device_context_->UpdateSubresource(constant_buffer, 0, nullptr, &cb, 0, 0);
            game.device_context_->VSSetConstantBuffers(0, 1, &constant_buffer);
            // Rasterizer Stage.
            game.device_context_->RSSetState(rasterizerState);
            game.device_context_->RSSetViewports(1, &game.vp_);
            // Pixel Shader.
            game.device_context_->PSSetShader(pixel_shader, nullptr, 0);
            if (ps_texture0)
            {
                game.device_context_->PSSetShaderResources(0, 1, &ps_texture0);
            }
            game.device_context_->PSSetSamplers(0, 1, &sampler_linear);
            // Output Merger.
            game.device_context_->OMSetRenderTargets(1, &game.render_target_view_, game.depth_buffer_);

            // Actual draw call.
            game.device_context_->DrawIndexed(render_mesh.indices_count, 0, 0);
        }

        // Present.
        const UINT SyncInterval = 1; // Synchronize presentation after single vertical blank.
        const HRESULT ok = game.swap_chain_->Present(SyncInterval, 0);
        Panic(SUCCEEDED(ok));
    }

    game.device_context_->ClearState();
    game.render_target_view_->Release();
    game.swap_chain_->Release();
    game.device_context_->Release();
    game.device_->Release();
    vertex_layout->Release();
    vertex_shader->Release();
    pixel_shader->Release();
    constant_buffer->Release();
    rasterizerState->Release();
    game.depth_buffer_->Release();

    return 0;
}
