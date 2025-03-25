#include "app_state.h"
#include "shaders_database.h"

#include <algorithm>
#include <filesystem>

#include <glm/gtc/epsilon.hpp>

/*static*/ Shaders Shaders::Build()
{
    const VSShader vs_shaders[] = {
        {&c_vs_gooch_shading},
        {&c_vs_basic_phong},
        {&c_vs_lines},
        {&c_vs_vertices_only},
        {&c_vs_normals},
    };
    const PSShader ps_shaders[] = {
        {&c_ps_gooch_shading},
        {&c_ps_basic_phong},
        {&c_ps_lines},
        {&c_ps_vertices_only},
        {&c_ps_normals},
    };

    Shaders all;
    all.vs_shaders_.assign(std::begin(vs_shaders), std::end(vs_shaders));
    all.ps_shaders_.assign(std::begin(ps_shaders), std::end(ps_shaders));
    return all;
}

const VSShader* Shaders::find_vs(const ShaderInfo& info) const
{
    Panic(info.kind == ShaderInfo::VS);
    auto it =
        std::find_if(vs_shaders_.cbegin(), vs_shaders_.cend(), [&](const VSShader& vs) { return vs.vs_info == &info; });
    if (it != vs_shaders_.cend())
    {
        return &(*it);
    }
    return nullptr;
}

const PSShader* Shaders::find_ps(const ShaderInfo& info) const
{
    Panic(info.kind == ShaderInfo::PS);
    auto it =
        std::find_if(ps_shaders_.cbegin(), ps_shaders_.cend(), [&](const PSShader& vs) { return vs.ps_info == &info; });
    if (it != ps_shaders_.cend())
    {
        return &(*it);
    }
    return nullptr;
}

void Init_KnownShaders(AppState& app)
{
    for (std::size_t index = 0, count = app.all_shaders_.vs_shaders_.size(); index < count; ++index)
    {
        VSShader& vs = app.all_shaders_.vs_shaders_[index];
        app.compiler_.create_vs(*app.device_.Get(), *vs.vs_info, vs.vs, vs.vs_layout);
        app.watch_.watch_changes_to(*vs.vs_info, reinterpret_cast<const void*>(index));
    }
    for (std::size_t index = 0, count = app.all_shaders_.ps_shaders_.size(); index < count; ++index)
    {
        PSShader& ps = app.all_shaders_.ps_shaders_[index];
        app.compiler_.create_ps(*app.device_.Get(), *ps.ps_info, ps.ps);
        app.watch_.watch_changes_to(*ps.ps_info, reinterpret_cast<const void*>(index));
    }
}

Camera::Camera()
{
    const float x = cosf(camera_yaw_) * cosf(camera_pitch_);
    const float y = sinf(camera_pitch_);
    const float z = sinf(camera_yaw_) * cosf(camera_pitch_);

    camera_front_dir_ = glm::normalize(glm::vec3(x, y, z));
}

void Camera::update(int x_delta, int y_delta)
{
    const float d_yaw = glm::radians(float(x_delta) * 0.05f);
    const float d_pitch = glm::radians(float(y_delta) * 0.05f);

    camera_yaw_ -= d_yaw;
    camera_pitch_ -= d_pitch;
    camera_pitch_ = std::clamp(camera_pitch_, glm::radians(-89.0f), glm::radians(89.0f));

    const float x = cosf(camera_yaw_) * cosf(camera_pitch_);
    const float y = sinf(camera_pitch_);
    const float z = sinf(camera_yaw_) * cosf(camera_pitch_);

    camera_front_dir_ = glm::normalize(glm::vec3(x, y, z));
}

glm::mat4x4 Camera::view() const
{
    return glm::lookAtLH(camera_position_, camera_position_ + camera_front_dir_, camera_up_dir_);
}

glm::vec3 Camera::right_dir() const
{
    return glm::normalize(glm::cross(camera_up_dir_, camera_front_dir_));
}

// Virtual-Key Codes:
// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
void TickInput(AppState& app)
{
    static const float camera_move_XZ_speed_ = 0.1f;
    static const float camera_move_Y_speed_ = 0.05f;
    glm::vec3 dp = glm::vec3(0.0f);
    if (app.keys_down_.contains(0x57)) // W
    {
        dp = app.camera_.camera_front_dir_ * camera_move_XZ_speed_;
    }
    if (app.keys_down_.contains(0x53)) // S
    {
        dp = -1.0f * app.camera_.camera_front_dir_ * camera_move_XZ_speed_;
    }
    if (app.keys_down_.contains(0x41)) // A
    {
        dp = -1.0f * app.camera_.right_dir() * camera_move_XZ_speed_;
    }
    if (app.keys_down_.contains(0x44)) // D
    {
        dp = app.camera_.right_dir() * camera_move_XZ_speed_;
    }
    if (app.keys_down_.contains(0x51)) // Q
    {
        dp = -1.0f * app.camera_.camera_up_dir_ * camera_move_Y_speed_;
    }
    if (app.keys_down_.contains(0x45)) // E
    {
        dp = app.camera_.camera_up_dir_ * camera_move_Y_speed_;
    }
    app.camera_.camera_position_ += dp;
}

// Handling Window Resizing:
// https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi?redirectedfrom=MSDN#handling-window-resizing
static void OnWindowResize(AppState& app, float width, float height)
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

static void OnWindowMouseInput(AppState& app, HRAWINPUT handle)
{
    RAWINPUT raw{};
    UINT size = sizeof(raw);
    const UINT status = ::GetRawInputData(handle, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));
    Panic(status == size);
    Panic(raw.header.dwType == RIM_TYPEMOUSE);

    // https://github.com/libsdl-org/SDL/blob/b45ed98ae936298163c0c85711c78d9ee3f52c27/src/video/windows/SDL_windowsevents.c#L586
    RAWMOUSE& mouse = raw.data.mouse;
    /*const*/ int dx = int(mouse.lLastX);
    /*const*/ int dy = int(mouse.lLastY);
    const bool is_absolute = ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0);
    const bool has_motion = (dx || dy);
    if (has_motion)
    {
        if (is_absolute)
        {
            // bool remote_desktop = (GetSystemMetrics(SM_REMOTESESSION) == TRUE);
            const bool virtual_desktop = ((mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0);
            const bool raw_coordinates = ((mouse.usFlags & 0x40) != 0);
            const int w = ::GetSystemMetrics(virtual_desktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
            const int h = ::GetSystemMetrics(virtual_desktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
            const int x = (raw_coordinates ? dx : (int)(((float)dx / 65535.0f) * w));
            const int y = (raw_coordinates ? dy : (int)(((float)dy / 65535.0f) * h));
            if ((app.last_raw_mouse_position.x == 0) && (app.last_raw_mouse_position.y == 0))
            {
                app.last_raw_mouse_position.x = x;
                app.last_raw_mouse_position.y = y;
            }
            dx = (x - app.last_raw_mouse_position.x);
            dy = (y - app.last_raw_mouse_position.y);
            app.last_raw_mouse_position = glm::ivec2(x, y);
        }
        else
        {
            // done, relative
        }
    }

    if (raw.data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
    {
        app.update_camera_ = true;
    }
    if (raw.data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
    {
        app.update_camera_ = false;
    }

    if (app.update_camera_)
    {
        app.camera_.update(dx, dy);
    }
}

void Init_MessageHandling(AppState& app)
{
    app.window_.on_message(WM_PAINT, [](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT {
        PAINTSTRUCT ps;
        (void)::BeginPaint(hwnd, &ps);
        (void)::EndPaint(hwnd, &ps);
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
    app.window_.on_message(WM_DESTROY, [](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT {
        ::PostQuitMessage(0);
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
    app.window_.on_message(WM_SIZE, [&app](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT {
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
    app.window_.on_message(WM_MOUSEWHEEL, [&app](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT {
        if (ImGui::GetIO().WantCaptureMouse)
        {
            // ImGui is in priority.
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        }
        const float delta_wheel = GET_WHEEL_DELTA_WPARAM(wparam);
        const float dv = (delta_wheel / WHEEL_DELTA) * app.mouse_scroll_sensitivity_;
        app.fov_y_ = std::clamp(app.fov_y_ - dv, glm::radians(1.f), glm::radians(90.f));
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });

    app.window_.on_message(WM_INPUT, [&app](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT {
        if (ImGui::GetIO().WantCaptureMouse)
        {
            // ImGui is in priority.
            return ::DefWindowProc(hwnd, message, wparam, lparam);
        }
        OnWindowMouseInput(app, HRAWINPUT(lparam));
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
    app.window_.on_message(WM_KEYUP, [&app](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT {
        // if (!ImGui::GetIO().WantCaptureKeyboard)
        {
            (void)app.keys_down_.erase(wparam);
        }
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
    app.window_.on_message(WM_KEYDOWN, [&app](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT {
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
    app.window_.on_message(WM_DROPFILES, [&app](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT {
        HDROP drop = reinterpret_cast<HDROP>(wparam);
        struct ScopeEnd
        {
            HDROP _drop;
            ~ScopeEnd()
            {
                ::DragFinish(_drop);
            }
        } _{drop};
        UINT files_count = ::DragQueryFileA(drop, UINT(0xFFFFFFFF), nullptr, 0);
        Panic(files_count != 0);
        for (UINT file_index = 0; file_index < files_count; ++file_index)
        {
            UINT length = ::DragQueryFileA(drop, file_index, nullptr, 0);
            Panic(length != 0);
            std::string file;
            file.resize(length + 1);
            length = ::DragQueryFileA(drop, file_index, &file[0], UINT(file.size()));
            Panic(length != 0);
            file.resize(length);
            app.files_to_load_.push_back(std::move(file));
        }
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
}

template <typename T>
static void RemoveDuplicates(std::vector<T>& es)
{
    std::sort(std::begin(es), std::end(es));
    auto it = std::unique(std::begin(es), std::end(es));
    es.erase(it, std::end(es));
}

static std::string GetPathFileName(const std::string& path)
{
    auto name = std::filesystem::path(path).stem();
    while (name.has_extension())
    {
        auto next = name.stem();
        if (next == name)
        {
            break;
        }
        name = std::move(next);
    }
    return name.string();
}

static void TryLoadModelsFromPaths(AppState& app, std::vector<std::string> paths)
{
    std::vector<std::string> folders;
    std::vector<std::string> files;
    for (std::string& p : paths)
    {
        std::error_code ec;
        std::filesystem::path path(p);
        if (std::filesystem::is_directory(path, ec))
        {
            folders.push_back(std::move(p));
        }
        else if (std::filesystem::is_regular_file(path, ec))
        {
            files.push_back(path.make_preferred().string());
            // We also want to get all files in the same directory.
            folders.push_back(path.parent_path().make_preferred().string());
        }
        Panic(!ec);
    }
    RemoveDuplicates(folders);
    // Collect all files in a folder.
    for (std::string& f : folders)
    {
        for (const auto& e : std::filesystem::recursive_directory_iterator(f))
        {
            if (e.is_regular_file())
            {
                auto path = e.path();
                files.push_back(path.make_preferred().string());
            }
        }
    }
    RemoveDuplicates(files);

    std::vector<FileModel> models;
    for (std::string& f : files)
    {
        const std::string ext = std::filesystem::path(f).extension().string();
        if (ext != ".obj")
        {
            continue;
        }
        auto maybe_model = LoadModel(f.c_str());
        if (!maybe_model)
        {
            continue;
        }
        models.push_back({});
        FileModel& fm = models.back();
        fm.file_name = std::move(f);
        fm.name = GetPathFileName(fm.file_name);
        fm.model = std::move(maybe_model.value());
    }

    std::vector<FileModel> old_models = std::move(app.models_);
    app.models_ = std::move(models);
    auto has_model = [&](const std::string& file_name) {
        for (const FileModel& m : app.models_)
        {
            if (m.file_name == file_name)
            {
                return true;
            }
        }
        return false;
    };
    for (FileModel& old_model : old_models)
    {
        if (!has_model(old_model.file_name))
        {
            app.models_.push_back(std::move(old_model));
        }
    }
}

bool TickModelsLoad(AppState& app)
{
    bool refresh = false;
    if (app.files_to_load_.size() > 0)
    {
        const bool force_select = (app.files_to_load_.size() == 1) //
                                  && (std::filesystem::is_regular_file(app.files_to_load_[0]));
        const auto select_file = std::filesystem::path(app.files_to_load_[0]).make_preferred().string();

        TryLoadModelsFromPaths(app, std::exchange(app.files_to_load_, {}));

        // If we dropped single file, select it for convenience.
        if (force_select)
        {
            for (std::size_t i = 0, count = app.models_.size(); i < count; ++i)
            {
                if (app.models_[i].file_name == select_file)
                {
                    app.imgui_.selected_model_index_ = int(i);
                    break;
                }
            }
        }
        refresh = true;
    }

    if (app.imgui_.selected_model_index_ != app.active_model_index_)
    {
        refresh = true;
    }

    // Model's change from the UI/initial change.
    if (refresh)
    {
        app.active_model_index_ = app.imgui_.selected_model_index_;
        const Model& model = app.models_[std::size_t(app.imgui_.selected_model_index_)].model;

        app.active_model_ = RenderModel::make(*app.device_.Get(), model);
        app.active_model_.vs_shader_ = &app.all_shaders_.vs_shaders_[app.imgui_.model_vs_index];
        app.active_model_.ps_shader_ = &app.all_shaders_.ps_shaders_[app.imgui_.model_ps_index];
        return true;
    }
    return false;
}

void TickShadersChange(AppState& app)
{
    auto patches = app.watch_.collect_changes(*app.device_.Get());
    if (patches.empty())
    {
        return;
    }
    for (ShaderPatch& patch : patches)
    {
        const std::size_t index = reinterpret_cast<std::size_t>(patch.user_data);
        switch (patch.shader_info->kind)
        {
        case ShaderInfo::VS:
        {
            Panic(index < app.all_shaders_.vs_shaders_.size());
            VSShader& shader = app.all_shaders_.vs_shaders_[index];
            shader.vs = std::move(patch.vs_shader);
            shader.vs_layout = std::move(patch.vs_layout);
            break;
        }
        case ShaderInfo::PS:
        {
            Panic(index < app.all_shaders_.ps_shaders_.size());
            PSShader& shader = app.all_shaders_.ps_shaders_[index];
            shader.ps = std::move(patch.ps_shader);
            break;
        }
        }
    }
}
