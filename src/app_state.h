#pragma once
#include "dx_api.h"
#include "imgui_state_debug.h"
#include "render_model.h"
#include "shaders_compiler.h"
#include "stub_window.h"

#include <string>
#include <unordered_set>
#include <vector>

struct AppState;

void Init_MessageHandling(AppState& app);
void Init_KnownShaders(AppState& app);

void TickInput(AppState& app);
bool TickModelsLoad(AppState& app);
void TickShadersChange(AppState& app);

struct Shaders
{
    static Shaders Build();

    const VSShader* find_vs(const ShaderInfo& info) const;
    const PSShader* find_ps(const ShaderInfo& info) const;

    std::vector<VSShader> vs_shaders_;
    std::vector<PSShader> ps_shaders_;
};

struct Camera
{
    Camera();

    glm::vec3 camera_position_ = glm::vec3(0.0f, 0.0f, -3.0f);
    glm::vec3 camera_up_dir_ = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 camera_front_dir_ = glm::vec3(0.0f, 0.0f, 1.0f);
    float camera_yaw_ = glm::radians(90.f);  // [-180; 180]
    float camera_pitch_ = glm::radians(0.f); // [-90; 90]

    glm::mat4x4 view() const;
    glm::vec3 right_dir() const;

    void update(int x_delta, int y_delta);
};

struct AppState
{
    StubWindow window_;
    ShadersCompiler compiler_;
    ShadersWatch watch_;
    Shaders all_shaders_;
    RenderModel active_model_;
    ImGuiState imgui_;
    Camera camera_;
    bool update_camera_ = false;

    float fov_y_ = glm::radians(45.f);
    float mouse_scroll_sensitivity_ = 0.05f;

    float window_width_ = 0.f;
    float window_height_ = 0.f;

    glm::ivec2 last_raw_mouse_position = glm::ivec2(0);
    std::unordered_set<WPARAM> keys_down_;

    D3D11_VIEWPORT vp_{};
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> device_context_;
    ComPtr<IDXGISwapChain> swap_chain_;
    ComPtr<ID3D11RenderTargetView> render_target_view_;
    ComPtr<ID3D11DepthStencilView> depth_buffer_;

    std::vector<std::string> files_to_load_;
    std::vector<FileModel> models_;
    int active_model_index_ = -1;
};
