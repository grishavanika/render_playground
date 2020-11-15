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

void TickInput(AppState& app);
void AddMessageHandling(AppState& app);
void OnWindowResize(AppState& app, float width, float height);
void OnWindowMouseInput(AppState& app, HRAWINPUT handle);

struct AllKnownShaders
{
    std::vector<VSShader> vs_shaders_;
    std::vector<PSShader> ps_shaders_;

    static AllKnownShaders BuildKnownAtCompileTime();
};

struct AppState
{
    StubWindow window_;
    ShadersCompiler compiler_;
    ShadersWatch watch_;
    AllKnownShaders known_shaders_;
    RenderModel active_model_;
    ImGuiState imgui_;

    float fov_y_ = DegreesToRadians(45.f);
    float window_width_ = 0.f;
    float window_height_ = 0.f;
    float mouse_scroll_sensitivity_ = 0.05f;
    float camera_yaw_degrees_ = 90.f;  // [-180; 180]
    float camera_pitch_degrees_ = 0.f; // [-90; 90]
    float camera_rotation_mouse_sensitivity_ = 0.06f;
    float camera_move_speed_ = 0.2f;

    std::unordered_set<WPARAM> keys_down_;

    glm::vec3 camera_position_ = glm::vec3(0.0f, 0.0f, -15.0f);
    glm::vec3 camera_up_dir_ = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 camera_front_dir_ = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 camera_right_dir_ = glm::vec3(1.0f, 0.0f, 0.0f);

    D3D11_VIEWPORT vp_{};
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> device_context_;
    ComPtr<IDXGISwapChain> swap_chain_;
    ComPtr<ID3D11RenderTargetView> render_target_view_;
    ComPtr<ID3D11DepthStencilView> depth_buffer_;

    std::string model_to_load_file_;
};