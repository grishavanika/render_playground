#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>

#include "dx_api.h"

#include "stub_window.h"
#include "utils.h"
#include "imgui_debug.h"

#include "render_lines.h"
#include "render_model.h"
#include "render_vertices_only.h"
#include "render_with_normals.h"

#include "shaders_compiler.h"
#include "shaders_database.h"

#include "predefined_objects.h"

#include <cstdlib>

#include <vector>
#include <unordered_set>

// Integration of ImGui comes from
// imgui-src/examples/example_win32_directx11/main.cpp
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

using namespace DirectX;

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
    float camera_move_speed_ = 0.2f;

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

void AddMessageHandling(StubWindow& window, GameState& game)
{
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
}

static void TickInput(GameState& game)
{
    if (game.keys_down_.contains(0x57)) // W
    {
        game.camera_position_ += (game.camera_front_dir_ * game.camera_move_speed_);
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
        (void)ImGui::Checkbox("Show cube (with normals)", &imgui.show_cube_normals);

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

struct AllKnownShaders
{
    std::vector<VSShader> vs_shaders_;
    std::vector<PSShader> ps_shaders_;

    static AllKnownShaders Build()
    {
        VSShader vs_shaders[] =
        {
            {&c_vs_basic_phong},
            {&c_vs_lines},
            {&c_vs_vertices_only},
            {&c_vs_normals},
        };
        PSShader ps_shaders[] =
        {
            {&c_ps_basic_phong},
            {&c_ps_lines},
            {&c_ps_vertices_only},
            {&c_ps_normals},
        };

        AllKnownShaders all;
        all.vs_shaders_.assign(std::begin(vs_shaders), std::end(vs_shaders));
        all.ps_shaders_.assign(std::begin(ps_shaders), std::end(ps_shaders));
        return all;
    }
};

template<typename RenderObject>
static void SetShadersRef(RenderObject& o, AllKnownShaders& all_shaders
    , const ShaderInfo& vs, const ShaderInfo& ps)
{
    for (VSShader& shader : all_shaders.vs_shaders_)
    {
        if (shader.vs_info == &vs)
        {
            o.vs_shader_ = &shader;
            break;
        }
    }
    for (PSShader& shader : all_shaders.ps_shaders_)
    {
        if (shader.ps_info == &ps)
        {
            o.ps_shader_ = &shader;
            break;
        }
    }

    Panic(o.vs_shader_);
    Panic(o.ps_shader_);
}

#define XX_LIGHT_MOVING() 1

struct Box3f
{
    Vector3f min_;
    Vector3f max_;
};

// https://stackoverflow.com/a/58630206
// http://www.realtimerendering.com/resources/GraphicsGems/gems/TransBox.c
static void Transform_Box(const XMMATRIX& M, const XMVECTOR& T, const Box3f& A, Box3f* B)
{
    float  a, b;
    float  Amin[3], Amax[3];
    float  Bmin[3], Bmax[3];
    int    i, j;

    /*Copy box A into a min array and a max array for easy reference.*/

    Amin[0] = A.min_.x;  Amax[0] = A.max_.x;
    Amin[1] = A.min_.y;  Amax[1] = A.max_.y;
    Amin[2] = A.min_.z;  Amax[2] = A.max_.z;

    /* Take care of translation by beginning at T. */

    Bmin[0] = Bmax[0] = XMVectorGetByIndex(T, 0);
    Bmin[1] = Bmax[1] = XMVectorGetByIndex(T, 1);
    Bmin[2] = Bmax[2] = XMVectorGetByIndex(T, 2);

    /* Now find the extreme points by considering the product of the */
    /* min and max with each component of M.  */
    
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            const float v_ = XMVectorGetByIndex(M.r[i], j);
            a = (v_ * Amin[j]);
            b = (v_ * Amax[j]);
            if (a < b)
            {
                Bmin[i] += a;
                Bmax[i] += b;
            }
            else
            {
                Bmin[i] += b;
                Bmax[i] += a;
            }
        }
    }

    /* Copy the result into the new box. */

    B->min_.x = Bmin[0];  B->max_.x = Bmax[0];
    B->min_.y = Bmin[1];  B->max_.y = Bmax[1];
    B->min_.z = Bmin[2];  B->max_.z = Bmax[2];
}

// I'm not smart, doing what stackoverflow says:
// break 4x4 into 3x3 transform and translation
// then apply Transform_Box() from "Graphics Gems", 1990.
static Box3f Transform_Box(const Box3f& a, const XMMATRIX& t)
{
    XMVECTOR scale;
    XMVECTOR quaternion;
    XMVECTOR T;
    const bool ok = XMMatrixDecompose(&scale, &quaternion, &T, t);
    Panic(ok);

    XMMATRIX M = XMMatrixRotationQuaternion(quaternion) * XMMatrixScalingFromVector(scale);

    Box3f B;
    Transform_Box(XMMatrixTranspose(M), T, a, &B);
    return B;
}

int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
    // const char* const obj = XX_PACKAGE_FOLDER R"(backpack.lr.bin)";
    const char* const obj = XX_PACKAGE_FOLDER R"(skull.lr.bin)";
    // const char* const obj = XX_PACKAGE_FOLDER R"(T-Rex.lr.bin)";

    Model model = LoadModel(obj);
    const Box3f model_aabb{model.aabb_min_, model.aabb_max_};

    GameState game;
    StubWindow window("xxx_lr");
    AddMessageHandling(window, game);

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
    AllKnownShaders known_shaders = AllKnownShaders::Build();

    RenderModel render_model = RenderModel::make(*game.device_.Get(), model);
    RenderLines render_lines = RenderLines::make(game.device_);
    RenderLines render_aabb = RenderLines::make(game.device_);
    RenderVertices render_cube = make_cube_vertices_only(game.device_);
    RenderWithNormals render_cube_normals = make_cube_with_normals(game.device_);

#if (0)
    SetShadersRef(render_model,        known_shaders, c_vs_basic_phong,   c_ps_basic_phong);
#else
    SetShadersRef(render_model,        known_shaders, c_vs_normals,       c_ps_normals);
#endif
    SetShadersRef(render_lines,        known_shaders, c_vs_lines,         c_ps_lines);
    SetShadersRef(render_aabb,         known_shaders, c_vs_lines,         c_ps_lines);
    SetShadersRef(render_cube,         known_shaders, c_vs_vertices_only, c_ps_vertices_only);
    SetShadersRef(render_cube_normals, known_shaders, c_vs_normals,       c_ps_normals);

    for (VSShader& vs : known_shaders.vs_shaders_)
    {
        compiler.create_vs(*game.device_.Get(), *vs.vs_info, vs.vs, vs.vs_layout);
        watch.watch_changes_to(*vs.vs_info);
    }
    for (PSShader& ps : known_shaders.ps_shaders_)
    {
        compiler.create_ps(*game.device_.Get(), *ps.ps_info, ps.ps);
        watch.watch_changes_to(*ps.ps_info);
    }

    render_lines.add_bb(BoundingBox(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(1.f, 1.f, 1.f)));
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
            for (VSShader& vs : known_shaders.vs_shaders_)
            {
                auto patch = watch.fetch_latest(*vs.vs_info);
                if (patch.vs_shader)
                {
                    vs.vs = std::move(patch.vs_shader);
                    vs.vs_layout = std::move(patch.vs_layout);
                    Panic(vs.vs);
                    Panic(vs.vs_layout);
                }
            }
            for (PSShader& ps : known_shaders.ps_shaders_)
            {
                auto patch = watch.fetch_latest(*ps.ps_info);
                if (patch.ps_shader)
                {
                    ps.ps = std::move(patch.ps_shader);
                    Panic(ps.ps);
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
            , 10000.0f); // FarZ
        const float t = float(::GetTickCount() - start_time) / 1000.f;

        view = XMMatrixLookAtLH(game.camera_position_
            , game.camera_position_ + game.camera_front_dir_
            , game.camera_up_dir_);

        render_model.world = XMMatrixIdentity()
            * game.imgui_.get_model_scale()
            * game.imgui_.get_model_rotation();
        { // Re-calculate box properly.
            const Box3f box = Transform_Box(model_aabb, render_model.world);
            render_aabb.clear();
            render_aabb.add_aabb(box.min_, box.max_, XMFLOAT3(1.f, 0.f, 0.f));
        }

        render_model.light_color = game.imgui_.get_light_color();
        render_model.viewer_position = game.camera_position_;
#if (XX_LIGHT_MOVING())
        const float radius = 10.0f;
        const float cam_x = (sinf(t) * radius);
        const float cam_z = (cosf(t) * radius);
        render_model.light_position = XMVectorSet(cam_x, 0.0f, cam_z, 0.0f);
#else
        render_model.light_position = game.camera_position_;
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
        const float c_clear_color[4] = {1.f, 1.f, 1.0f, 1.0f};
        game.device_context_->ClearRenderTargetView(game.render_target_view_.Get(), c_clear_color);
        game.device_context_->ClearDepthStencilView(game.depth_buffer_.Get()
            , D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        // Output Merger.
        game.device_context_->OMSetRenderTargets(1, game.render_target_view_.GetAddressOf(), game.depth_buffer_.Get());
        // Rasterizer Stage.
        game.device_context_->RSSetState(rasterizer_state.Get());
        game.device_context_->RSSetViewports(1, &game.vp_);

        if (game.imgui_.show_cube_normals)
        {
            // XMMatrixTranspose(XMMatrixIdentity());
            render_cube_normals.render(*game.device_context_.Get()
                , XMMatrixTranspose(view)
                , XMMatrixTranspose(projection));
        }
        if (game.imgui_.show_cube)
        {
            // XMMatrixTranspose(XMMatrixIdentity());
#if (XX_LIGHT_MOVING())
            render_cube.world = XMMatrixTranspose(XMMatrixTranslation(cam_x, 0, cam_z));
#endif
            render_cube.render(*game.device_context_.Get()
                , XMMatrixTranspose(view)
                , XMMatrixTranspose(projection));
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
            render_aabb.render(*game.device_context_.Get()
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
