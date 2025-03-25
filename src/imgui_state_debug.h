#pragma once
#include "utils.h"
#include "imgui.h"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <vector>

struct AppState;
struct ImGuiState;
struct RenderModel;

void ImGui_Setup(AppState& app);
void ImGui_TweaksInput(ImGuiState& imgui);
void ImGui_ShadersInput(ImGuiState& imgui);
void TickImGui(AppState& app);

enum class LightMode : int
{
    Static_AtCameraPosition,
    Moving_Active,
    Moving_Paused,
};

struct ImGuiState
{
    bool show_demo_window = false;
    bool show = false;

    // Lighting.
    glm::vec3 light_color = glm::vec3(1.f);

    // Model.
    float model_scale = 1.f;

    int model_vs_index = 0;
    int model_ps_index = 0;

    // Render config.
    bool wireframe = false;
    bool need_change_wireframe = false;
    bool show_model = true;
    bool show_zero_world_space = false;
    bool show_light_cube = false;
    LightMode light_mode = LightMode::Moving_Active;
    float light_move_radius = 10.f;
    bool show_cube_normals = false;
    bool enable_camera_rotation = false;
    bool enable_model_rotation = true;

    AppState* app_ = nullptr;
    int selected_model_index_ = 0;

    bool check_wireframe_change()
    {
        if (need_change_wireframe)
        {
            need_change_wireframe = false;
            return true;
        }
        return false;
    }
    glm::mat4x4 get_model_scale() const
    {
        return glm::scale(glm::mat4x4(1.f), glm::vec3(model_scale));
    }
};
