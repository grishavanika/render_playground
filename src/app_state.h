#pragma once
#include "dx_api.h"
#include "stub_window.h"
#include "imgui_state_debug.h"
#include "shaders_compiler.h"
#include "render_model.h"

#include <unordered_set>
#include <string>
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

struct AppState
{
    StubWindow window_;
    ShadersCompiler compiler_;
    ShadersWatch watch_;
    Shaders all_shaders_;
    RenderModel active_model_;
    ImGuiState imgui_;

    glm::mat4x4 model_rotation_ = glm::mat4x4(1.f);
    bool is_model_rotation_active_ = false;

    float fov_y_ = glm::radians(45.f);
    float window_width_ = 0.f;
    float window_height_ = 0.f;
    float mouse_scroll_sensitivity_ = 0.05f;
    float camera_yaw_ = glm::radians(90.f);  // [-180; 180]
    float camera_pitch_ = glm::radians(0.f); // [-90; 90]
    float camera_rotation_sensitivity_ = 0.05f;
    float model_rotation_sensitivity_ = 0.25f;
    float camera_move_XZ_speed_ = 0.1f;
    float camera_move_Y_speed_ = 0.05f;

    std::unordered_set<WPARAM> keys_down_;

    glm::vec3 camera_position_ = glm::vec3(0.0f, 0.0f, -3.0f);
    glm::vec3 camera_up_dir_ = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 camera_front_dir_ = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 camera_right_dir_ = glm::vec3(1.0f, 0.0f, 0.0f);

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
