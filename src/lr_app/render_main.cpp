#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>

#pragma warning(push)
// macro redefinition
#pragma warning(disable : 4005)
#include <d3d11.h>
#pragma warning(pop)

#include <DirectXMath.h>
using namespace DirectX;

// Integration of ImGui comes from
// imgui-src/examples/example_win32_directx11/main.cpp
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <cstdlib>

#include <vector>
#include <unordered_set>

#include "stub_window.h"
#include "utils.h"

#include "render_lines.h"
#include "render_model.h"
#include "render_vertices_only.h"

#include "shaders_compiler.h"
#include "shaders_database.h"

static const Vector3f c_cube_vertices[] =
{
    Vector3f{-0.5f, -0.5f, -0.5f},
    Vector3f{ 0.5f, -0.5f, -0.5f},
    Vector3f{ 0.5f,  0.5f, -0.5f},
    Vector3f{ 0.5f,  0.5f, -0.5f},
    Vector3f{-0.5f,  0.5f, -0.5f},
    Vector3f{-0.5f, -0.5f, -0.5f},
    Vector3f{-0.5f, -0.5f,  0.5f},
    Vector3f{ 0.5f, -0.5f,  0.5f},
    Vector3f{ 0.5f,  0.5f,  0.5f},
    Vector3f{ 0.5f,  0.5f,  0.5f},
    Vector3f{-0.5f,  0.5f,  0.5f},
    Vector3f{-0.5f, -0.5f,  0.5f},
    Vector3f{-0.5f,  0.5f,  0.5f},
    Vector3f{-0.5f,  0.5f, -0.5f},
    Vector3f{-0.5f, -0.5f, -0.5f},
    Vector3f{-0.5f, -0.5f, -0.5f},
    Vector3f{-0.5f, -0.5f,  0.5f},
    Vector3f{-0.5f,  0.5f,  0.5f},
    Vector3f{ 0.5f,  0.5f,  0.5f},
    Vector3f{ 0.5f,  0.5f, -0.5f},
    Vector3f{ 0.5f, -0.5f, -0.5f},
    Vector3f{ 0.5f, -0.5f, -0.5f},
    Vector3f{ 0.5f, -0.5f,  0.5f},
    Vector3f{ 0.5f,  0.5f,  0.5f},
    Vector3f{-0.5f, -0.5f, -0.5f},
    Vector3f{ 0.5f, -0.5f, -0.5f},
    Vector3f{ 0.5f, -0.5f,  0.5f},
    Vector3f{ 0.5f, -0.5f,  0.5f},
    Vector3f{-0.5f, -0.5f,  0.5f},
    Vector3f{-0.5f, -0.5f, -0.5f},
    Vector3f{-0.5f,  0.5f, -0.5f},
    Vector3f{ 0.5f,  0.5f, -0.5f},
    Vector3f{ 0.5f,  0.5f,  0.5f},
    Vector3f{ 0.5f,  0.5f,  0.5f},
    Vector3f{-0.5f,  0.5f,  0.5f},
    Vector3f{-0.5f,  0.5f, -0.5f}
};

struct ImGuiState
{
    bool show_demo_window = false;
    bool show = false;
    
    // Lighting.
    float light_power = 5.f;
    ImVec4 light_color = ImVec4(1.f, 1.f, 1.f, 1.f);

    // Model.
    float model_scale = 1.f;
    // Pitch, Yaw, Roll.
    // http://hugin.sourceforge.net/docs/manual/Image_positioning_model.html#:~:text=Positive%20Roll%20values%20mean%20the,%2B90%C2%B0%20(Zenith).
    ImVec4 model_rotation = ImVec4(0.f, 180.f, 0.f, 0.f);

    // Render config.
    bool wireframe = false;
    bool need_change_wireframe = false;
    bool show_model = false;
    bool show_zero_world_space = false;
    bool show_cube = true;
    bool enable_mouse = false;

    bool check_wireframe_change()
    {
        if (need_change_wireframe)
        {
            need_change_wireframe = false;
            return true;
        }
        return false;
    }
    XMVECTOR get_light_color() const
    {
        return light_power * XMVectorSet(light_color.x, light_color.y, light_color.z, 1.f);
    }
    XMMATRIX get_model_scale() const
    {
        return XMMatrixScaling(model_scale, model_scale, model_scale);
    }
    XMMATRIX get_model_rotation() const
    {
        const float pitch = DegreesToRadians(model_rotation.x);
        const float yaw   = DegreesToRadians(model_rotation.y);
        const float roll  = DegreesToRadians(model_rotation.z);
        return XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
    }
};

#pragma warning(push)
// structure was padded due to alignment specifier
#pragma warning(disable:4324)
struct GameState
{
    ImGuiState imgui_;

    float fov_y_ = DegreesToRadians(45.f);
    float aspect_ratio_ = 0.f;
    float mouse_scroll_sensitivity_ = 0.05f;
    float camera_yaw_degrees_ = 90.f;  // [-180; 180]
    float camera_pitch_degrees_ = 0.f; // [-90; 90]
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

static void TickInput(GameState& game)
{
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
    if (game.keys_down_.contains(0x4D)) // M
    {
        game.imgui_.enable_mouse = !game.imgui_.enable_mouse;
        game.keys_down_.erase(0x4D);
    }
}

static void TickImGui(GameState& game)
{
    if (game.imgui_.show_demo_window)
    {
        ImGui::ShowDemoWindow(&game.imgui_.show_demo_window);
    }
    if (!game.imgui_.show)
    {
        return;
    }

    ImGuiState& imgui = game.imgui_;
    if (ImGui::Begin("Tweaks", &imgui.show))
    {
        (void)ImGui::Checkbox("Enable Mouse (M)", &imgui.enable_mouse);
        ImGui::SameLine();
        imgui.need_change_wireframe = ImGui::Checkbox("Render wireframe", &imgui.wireframe);
        (void)ImGui::Checkbox("Show model", &imgui.show_model);
        (void)ImGui::Checkbox("Show zero world space", &imgui.show_zero_world_space);
        (void)ImGui::Checkbox("Show cube", &imgui.show_cube);

        (void)ImGui::SliderFloat("Light power", &imgui.light_power, 0.0f, 32.0f);
        (void)ImGui::ColorEdit3("Light color", (float*)&imgui.light_color, ImGuiColorEditFlags_NoAlpha);
        (void)ImGui::SliderFloat("Model scale", &imgui.model_scale, 0.01f, 32.f);
        
        (void)ImGui::SliderFloat3("Model rotation (Pitch, Yaw, Roll)", (float*)&imgui.model_rotation, -180., 180.f);
        imgui.model_rotation.x = std::clamp(imgui.model_rotation.x, -90.f, 90.f);

        (void)ImGui::SliderFloat3("Camera position", (float*)&game.camera_position_, -100.f, 100.f);

        (void)ImGui::Checkbox("ImGui Demo", &imgui.show_demo_window);
    }
    ImGui::End();
}

#if !defined(XX_PACKAGE_FOLDER)
#  error "Build system missed to specify where package (binaries/data) is."
#endif
#if !defined(XX_SHADERS_FOLDER)
#  error "Build system missed to specify where shaders are."
#endif

struct VSShader
{
    const ShaderInfo* vs_info;
    ComPtr<ID3D11VertexShader>* vs;
    ComPtr<ID3D11InputLayout>* vs_layout;
};

struct PSShader
{
    const ShaderInfo* ps_info;
    ComPtr<ID3D11PixelShader>* ps;
};

int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
#if (XX_HAS_TEXTURE_COORDS() && XX_HAS_NORMALS())
    const char* const obj = XX_PACKAGE_FOLDER R"(backpack.lr.bin)";
#elif (XX_HAS_NORMALS())
    const char* const obj = XX_PACKAGE_FOLDER R"(skull.lr.bin)";
#else
#  error "Find some predefined model with no Normals."
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
        if (ImGui::GetIO().WantCaptureMouse)
        {
            // ImGui is in priority.
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        }
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
        if (ImGui::GetIO().WantCaptureMouse || !game.imgui_.enable_mouse)
        {
            // ImGui is in priority.
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        }

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

        const float d_yaw = (float(x_delta) * game.camera_rotation_mouse_sensitivity_);
        const float d_pitch = (float(y_delta) * game.camera_rotation_mouse_sensitivity_);

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
        // if (!ImGui::GetIO().WantCaptureKeyboard)
        {
            (void)game.keys_down_.erase(wparam);
        }
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
    window.on_message(WM_KEYDOWN
        , [&game](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        // Make possible to hide ImGui even when it's active (wants keyboard).
        if (wparam == 0xc0) // `
        {
            game.imgui_.show = !game.imgui_.show;
        }
        // else if (!ImGui::GetIO().WantCaptureKeyboard)
        {
            (void)game.keys_down_.insert(wparam);
        }
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });

    Panic(::ShowWindow(window.wnd(), SW_SHOW) == 0/*was previously hidden*/);

    RECT client_rect{};
    Panic(!!::GetClientRect(window.wnd(), &client_rect));
    const UINT client_width = (client_rect.right - client_rect.left);
    const UINT client_height = (client_rect.bottom - client_rect.top);
    game.aspect_ratio_ = (float(client_width) / float(client_height));
    
    { // Calculate default camera orientation from initial angles.
        const float yaw = DegreesToRadians(game.camera_yaw_degrees_);
        const float pitch = DegreesToRadians(game.camera_pitch_degrees_);

        const float x = cosf(yaw) * cosf(pitch);
        const float y = sinf(pitch);
        const float z = sinf(yaw) * cosf(pitch);

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
    wfd.FillMode = D3D11_FILL_SOLID;
    wfd.CullMode = D3D11_CULL_NONE;
    wfd.DepthClipEnable = TRUE;
    wfd.AntialiasedLineEnable = TRUE;
    ComPtr<ID3D11RasterizerState> rasterizer_state;
    hr = game.device_->CreateRasterizerState(&wfd, &rasterizer_state);
    Panic(SUCCEEDED(hr));

    ///////////////////////////////////////////////////////////////////////////
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style.
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings.
    Panic(ImGui_ImplWin32_Init(window.wnd()));
    Panic(ImGui_ImplDX11_Init(game.device_.Get(), game.device_context_.Get()));

    // Forward declare message handler from imgui_impl_win32.cpp
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
        HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    window.set_message_handler(
        [](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam))
        {
            return TRUE;
        }
        return FALSE;
    });
    ///////////////////////////////////////////////////////////////////////////

    ShadersCompiler compiler;
    ShadersWatch watch(compiler);

    RenderModel render_model   = RenderModel::make(*game.device_.Get(), model);
    RenderLines render_lines   = RenderLines::make(game.device_);
    RenderVertices render_cube = RenderVertices::make(game.device_, c_cube_vertices);
    const VSShader vs_shaders[] =
    {
        {&c_vs_basic_phong,   std::addressof(render_model.vertex_shader_), std::addressof(render_model.vertex_layout_)},
        {&c_vs_lines,         std::addressof(render_lines.vertex_shader_), std::addressof(render_lines.vertex_layout_)},
        {&c_vs_vertices_only, std::addressof(render_cube.vertex_shader_),  std::addressof(render_cube.vertex_layout_)},
    };
    const PSShader ps_shaders[] =
    {
        {&c_ps_basic_phong,   std::addressof(render_model.pixel_shader_)},
        {&c_ps_lines,         std::addressof(render_lines.pixel_shader_)},
        {&c_ps_vertices_only, std::addressof(render_cube.pixel_shader_)},
    };

    for (const VSShader& vs : vs_shaders)
    {
        compiler.create_vs(*game.device_.Get(), *vs.vs_info, *vs.vs, *vs.vs_layout);
        watch.watch_changes_to(*vs.vs_info);
    }
    for (const PSShader& ps : ps_shaders)
    {
        compiler.create_ps(*game.device_.Get(), *ps.ps_info, *ps.ps);
        watch.watch_changes_to(*ps.ps_info);
    }

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

        if (watch.collect_changes(*game.device_.Get()) > 0)
        {
            for (const VSShader& vs : vs_shaders)
            {
                auto patch = watch.fetch_latest(*vs.vs_info);
                if (patch.vs_shader)
                {
                    *vs.vs = std::move(patch.vs_shader);
                    *vs.vs_layout = std::move(patch.vs_layout);
                    Panic(*vs.vs);
                    Panic(*vs.vs_layout);
                }
            }
            for (const PSShader& ps : ps_shaders)
            {
                auto patch = watch.fetch_latest(*ps.ps_info);
                if (patch.ps_shader)
                {
                    *ps.ps = std::move(patch.ps_shader);
                    Panic(*ps.ps);
                }
            }
        }

        // Start the Dear ImGui frame.
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        TickImGui(game);
        TickInput(game);

        projection = XMMatrixPerspectiveFovLH(game.fov_y_
            , game.aspect_ratio_
            , 0.01f    // NearZ
            , 100.0f); // FarZ
        const float t = float(::GetTickCount() - start_time) / 1000.f;

        view = XMMatrixLookAtLH(game.camera_position_
            , game.camera_position_ + game.camera_front_dir_
            , game.camera_up_dir_);

        render_model.world = XMMatrixIdentity()
            * game.imgui_.get_model_scale()
            * game.imgui_.get_model_rotation();

        render_model.light_color = game.imgui_.get_light_color();
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
        if (game.imgui_.check_wireframe_change())
        {
            wfd.FillMode = game.imgui_.wireframe
                ? D3D11_FILL_WIREFRAME
                : D3D11_FILL_SOLID;
            rasterizer_state.Reset();
            hr = game.device_->CreateRasterizerState(&wfd, &rasterizer_state);
            Panic(SUCCEEDED(hr));
        }

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

        if (game.imgui_.show_cube)
        {
            render_cube.render(*game.device_context_.Get()
#if (0)
                , XMMatrixTranspose(XMMatrixIdentity())
                , XMMatrixTranspose(XMMatrixIdentity())
#else
                , XMMatrixTranspose(view)
                , XMMatrixTranspose(projection)
#endif
            );
        }

        if (game.imgui_.show_zero_world_space)
        {
#if (0)
            render_lines.world = render_model.world;
#endif
            render_lines.render(*game.device_context_.Get()
                , XMMatrixTranspose(view)
                , XMMatrixTranspose(projection));
        }
        if (game.imgui_.show_model)
        {
            render_model.render(*game.device_context_.Get()
                , XMMatrixTranspose(view)
                , XMMatrixTranspose(projection));
        }

        // Rendering
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present.
        const UINT SyncInterval = 1; // Synchronize presentation after single vertical blank.
        hr = game.swap_chain_->Present(SyncInterval, 0);
        Panic(SUCCEEDED(hr));
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    return 0;
}
