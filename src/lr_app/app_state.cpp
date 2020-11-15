#include "app_state.h"
#include "shaders_database.h"

/*static*/ AllKnownShaders AllKnownShaders::BuildKnownAtCompileTime()
{
    VSShader vs_shaders[] =
    {
        {&c_vs_basic_phong},
        {&c_vs_gooch_shading},
        {&c_vs_lines},
        {&c_vs_vertices_only},
        {&c_vs_normals},
    };
    PSShader ps_shaders[] =
    {
        {&c_ps_basic_phong},
        {&c_ps_gooch_shading},
        {&c_ps_lines},
        {&c_ps_vertices_only},
        {&c_ps_normals},
    };

    AllKnownShaders all;
    all.vs_shaders_.assign(std::begin(vs_shaders), std::end(vs_shaders));
    all.ps_shaders_.assign(std::begin(ps_shaders), std::end(ps_shaders));
    return all;
}

// Virtual-Key Codes:
// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
void TickInput(AppState& app)
{
    if (app.keys_down_.contains(0x57)) // W
    {
        app.camera_position_ += (app.camera_front_dir_ * app.camera_move_speed_);
    }
    if (app.keys_down_.contains(0x53)) // S
    {
        app.camera_position_ -= (app.camera_front_dir_ * app.camera_move_speed_);
    }
    if (app.keys_down_.contains(0x41)) // A
    {
        app.camera_position_ += (app.camera_right_dir_ * app.camera_move_speed_);
    }
    if (app.keys_down_.contains(0x44)) // D
    {
        app.camera_position_ -= (app.camera_right_dir_ * app.camera_move_speed_);
    }
    if (app.keys_down_.contains(0x51)) // Q
    {
        app.camera_position_ -= (app.camera_up_dir_ * app.camera_move_speed_);
    }
    if (app.keys_down_.contains(0x45)) // E
    {
        app.camera_position_ += (app.camera_up_dir_ * app.camera_move_speed_);
    }
    if (app.keys_down_.contains(0x4D)) // M
    {
        app.imgui_.enable_mouse = !app.imgui_.enable_mouse;
        app.keys_down_.erase(0x4D);
    }
}

// Handling Window Resizing:
// https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi?redirectedfrom=MSDN#handling-window-resizing
void OnWindowResize(AppState& app, float width, float height)
{
    Panic(app.device_);
    Panic(app.swap_chain_);
    Panic(app.device_context_);
    Panic(app.render_target_view_);
    Panic(app.depth_buffer_);

    app.window_width_ = width;
    app.window_height_ = height;
    app.device_context_->OMSetRenderTargets(0, 0, 0);
    app.render_target_view_.Reset();
    // Preserve the existing buffer count and format.
    HRESULT hr = app.swap_chain_->ResizeBuffers(0, UINT(width), UINT(height), DXGI_FORMAT_UNKNOWN, 0);
    Panic(SUCCEEDED(hr));

    // Get buffer and create a render-target-view.
    ComPtr<ID3D11Texture2D> buffer;
    hr = app.swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &buffer);
    Panic(SUCCEEDED(hr));

    hr = app.device_->CreateRenderTargetView(buffer.Get(), nullptr, &app.render_target_view_);
    Panic(SUCCEEDED(hr));
    buffer.Reset();

    app.depth_buffer_.Reset();

    D3D11_TEXTURE2D_DESC depthTextureDesc{};
    depthTextureDesc.Width = UINT(width);
    depthTextureDesc.Height = UINT(height);
    depthTextureDesc.MipLevels = 1;
    depthTextureDesc.ArraySize = 1;
    depthTextureDesc.SampleDesc.Count = 4;
    depthTextureDesc.SampleDesc.Quality = 0;
    depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ComPtr<ID3D11Texture2D> depth_stencil_texture;
    hr = app.device_->CreateTexture2D(&depthTextureDesc, nullptr, &depth_stencil_texture);
    Panic(SUCCEEDED(hr));

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = depthTextureDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

    hr = app.device_->CreateDepthStencilView(depth_stencil_texture.Get(), &dsvDesc, &app.depth_buffer_);
    depth_stencil_texture.Reset();
    Panic(SUCCEEDED(hr));

    app.device_context_->OMSetRenderTargets(1, app.render_target_view_.GetAddressOf(), app.depth_buffer_.Get());

    // Set up the viewport.
    app.vp_.Width = width;
    app.vp_.Height = height;
}

// https://learnopengl.com/Getting-started/Camera
void OnWindowMouseInput(AppState& app, HRAWINPUT handle)
{
    RAWINPUT raw{};
    UINT size = sizeof(raw);
    const UINT status = ::GetRawInputData(handle
        , RID_INPUT
        , &raw
        , &size
        , sizeof(RAWINPUTHEADER));
    Panic(status == size);
    Panic(raw.header.dwType == RIM_TYPEMOUSE);
    Panic((raw.data.mouse.usFlags & MOUSE_MOVE_RELATIVE) == MOUSE_MOVE_RELATIVE);
    const LONG x_delta = raw.data.mouse.lLastX;
    const LONG y_delta = raw.data.mouse.lLastY;

    const float d_yaw = (float(x_delta) * app.camera_rotation_mouse_sensitivity_);
    const float d_pitch = (float(y_delta) * app.camera_rotation_mouse_sensitivity_);

    app.camera_yaw_degrees_ -= d_yaw;
    app.camera_pitch_degrees_ = std::clamp(app.camera_pitch_degrees_ - d_pitch, -89.0f, 89.0f);

    const float yaw = DegreesToRadians(app.camera_yaw_degrees_);
    const float pitch = DegreesToRadians(app.camera_pitch_degrees_);

    const float x = cosf(yaw) * cosf(pitch);
    const float y = sinf(pitch);
    const float z = sinf(yaw) * cosf(DegreesToRadians(app.camera_pitch_degrees_));

    app.camera_front_dir_ = glm::normalize(glm::vec3(x, y, z));
    app.camera_right_dir_ = glm::normalize(glm::cross(app.camera_front_dir_, app.camera_up_dir_));
}

void AddMessageHandling(AppState& app)
{
    app.window_.on_message(WM_PAINT
        , [](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            PAINTSTRUCT ps;
            (void)::BeginPaint(hwnd, &ps);
            (void)::EndPaint(hwnd, &ps);
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        });
    app.window_.on_message(WM_DESTROY
        , [](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            ::PostQuitMessage(0);
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        });
    app.window_.on_message(WM_SIZE
        , [&app](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            if (!app.device_)
            {
                // Window initialization; ignore.
                return ::DefWindowProc(hwnd, message, wparam, lparam);
            }
            if (wparam == SIZE_MINIMIZED)
            {
                return ::DefWindowProc(hwnd, message, wparam, lparam);
            }
            OnWindowResize(app, LOWORD(lparam), HIWORD(lparam));
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        });
    app.window_.on_message(WM_MOUSEWHEEL
        , [&app](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            if (ImGui::GetIO().WantCaptureMouse)
            {
                // ImGui is in priority.
                return ::DefWindowProc(hwnd, message, wparam, lparam);
            }
            const float delta_wheel = GET_WHEEL_DELTA_WPARAM(wparam);
            const float dv = (delta_wheel / WHEEL_DELTA) * app.mouse_scroll_sensitivity_;
            app.fov_y_ = std::clamp(app.fov_y_ - dv, DegreesToRadians(1.f), DegreesToRadians(90.f));
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        });

    app.window_.on_message(WM_INPUT
        , [&app](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            if (ImGui::GetIO().WantCaptureMouse || !app.imgui_.enable_mouse)
            {
                // ImGui is in priority.
                return ::DefWindowProc(hwnd, message, wparam, lparam);
            }
            OnWindowMouseInput(app, HRAWINPUT(lparam));
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        });
    app.window_.on_message(WM_KEYUP
        , [&app](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            // if (!ImGui::GetIO().WantCaptureKeyboard)
            {
                (void)app.keys_down_.erase(wparam);
            }
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        });
    app.window_.on_message(WM_KEYDOWN
        , [&app](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            // Make possible to hide ImGui even when it's active (wants keyboard).
            if (wparam == 0xc0) // `
            {
                app.imgui_.show = !app.imgui_.show;
            }
            // else if (!ImGui::GetIO().WantCaptureKeyboard)
            {
                (void)app.keys_down_.insert(wparam);
            }
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        });
    app.window_.on_message(WM_DROPFILES
        , [&app](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            HDROP drop = reinterpret_cast<HDROP>(wparam);
            UINT files_count = ::DragQueryFileA(drop, UINT(0xFFFFFFFF), nullptr, 0);
            Panic(files_count != 0);
            if (files_count > 1)
            {
                // We don't support multiple files drop.
                ::DragFinish(drop);
                return ::DefWindowProc(hwnd, message, wparam, lparam);
            }
            UINT length = ::DragQueryFileA(drop, UINT(0), nullptr, 0);
            Panic(length != 0);
            std::string file;
            file.resize(length + 1);
            UINT final_length = ::DragQueryFileA(drop, UINT(0), &file[0], UINT(file.size()));
            Panic(final_length != 0);
            file.resize(final_length);
            ::DragFinish(drop);

            app.model_to_load_file_ = std::move(file);
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        });
}
