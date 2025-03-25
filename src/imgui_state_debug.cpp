#include <cmath> // Fix external libraries warning/errors :(

#include "imgui_state_debug.h"
#include "app_state.h"
#include "render_model.h"

// Integration of ImGui comes from
// imgui-src/examples/example_win32_directx11/main.cpp
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

void ImGui_Setup(AppState& app)
{
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
	Panic(ImGui_ImplWin32_Init(app.window_.wnd()));
	Panic(ImGui_ImplDX11_Init(app.device_.Get(), app.device_context_.Get()));

	// Forward declare message handler from imgui_impl_win32.cpp
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	app.window_.set_message_handler(
		[](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
		{
			if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam))
			{
				return TRUE;
			}
			return FALSE;
		});
}

void ImGui_TweaksInput(ImGuiState& imgui)
{
	struct ScopeEnd { ~ScopeEnd() { ImGui::End(); } } _;
	if (!ImGui::Begin("Tweaks", &imgui.show))
	{
		return;
	}

	ImGui::Combo("Models", &imgui.selected_model_index_
		, [](void* data, int idx, const char** out_text)
		{
			auto& models = *static_cast<const std::vector<FileModel>*>(data);
			*out_text = models[std::size_t(idx)].name.c_str();
			return true;
		}
		, &imgui.app_->models_
			, int(imgui.app_->models_.size()));
	ImGui::SameLine();
	(void)ImGui::Checkbox("Show model", &imgui.show_model);

	(void)ImGui::Checkbox("Mouse movements (press M)", &imgui.enable_camera_rotation);
	(void)ImGui::Checkbox("Model rotation (with mouse middle button)", &imgui.enable_model_rotation);

	imgui.need_change_wireframe = ImGui::Checkbox("Render wireframe", &imgui.wireframe);
	(void)ImGui::Checkbox("Show zero world space (red = x, green = y, blue = z)", &imgui.show_zero_world_space);
	(void)ImGui::Checkbox("Show light cube", &imgui.show_light_cube);
	const char* items[] = { "Static_AtCameraPosition", "Moving_Active", "Moving_Paused" };
	ImGui::Combo("Light mode", reinterpret_cast<int*>(&imgui.light_mode), items, IM_ARRAYSIZE(items));
	(void)ImGui::SliderFloat("Light move radius", &imgui.light_move_radius, 0.01f, 32.0f);

	(void)ImGui::ColorEdit3("Light color", (float*)&imgui.light_color, ImGuiColorEditFlags_NoAlpha);
	(void)ImGui::SliderFloat("Model scale", &imgui.model_scale, 0.01f, 8.f);

	(void)ImGui::SliderFloat3("Camera position", (float*)&imgui.app_->camera_position_, -100.f, 100.f);

	ImGui::Separator();
	(void)ImGui::Checkbox("ImGui Demo", &imgui.show_demo_window);
}

void ImGui_ShadersInput(ImGuiState& imgui)
{
	struct ScopeEnd { ~ScopeEnd() { ImGui::End(); } } _;
	if (!ImGui::Begin("Shaders", &imgui.show))
	{
		return;
	}

	int used_vs_now = 0;
	int used_ps_now = 0;
	Shaders& shaders = imgui.app_->all_shaders_;
	RenderModel& active_model = imgui.app_->active_model_;

	ImGui::PushID("Vertex shaders");
	ImGui::BeginGroup();
	for (int index = 0, count = int(shaders.vs_shaders_.size()); index < count; ++index)
	{
		const VSShader& vs = shaders.vs_shaders_[std::size_t(index)];
		if (&vs == active_model.vs_shader_)
		{
			used_vs_now = index;
			break;
		}
	}
	for (int index = 0, count = int(shaders.vs_shaders_.size()); index < count; ++index)
	{
		const VSShader& vs = shaders.vs_shaders_[std::size_t(index)];
		if (used_vs_now == index)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 0.f, 1.f));
		}
		ImGui::RadioButton(vs.vs_info->debug_name, &imgui.model_vs_index, index);
		if (used_vs_now == index)
		{
			ImGui::PopStyleColor();
		}
	}
	ImGui::EndGroup();
	ImGui::PopID();
	ImGui::PushID("Pixel shaders");
	ImGui::SameLine();
	ImGui::BeginGroup();
	for (int index = 0, count = int(shaders.ps_shaders_.size()); index < count; ++index)
	{
		const PSShader& ps = shaders.ps_shaders_[std::size_t(index)];
		if (&ps == active_model.ps_shader_)
		{
			used_ps_now = index;
			break;
		}
	}
	for (int index = 0, count = int(shaders.ps_shaders_.size()); index < count; ++index)
	{
		const PSShader& ps = shaders.ps_shaders_[std::size_t(index)];
		if (used_ps_now == index)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 0.f, 1.f));
		}
		ImGui::RadioButton(ps.ps_info->debug_name, &imgui.model_ps_index, index);
		if (used_ps_now == index)
		{
			ImGui::PopStyleColor();
		}
	}
	ImGui::EndGroup();
	ImGui::PopID();
	ImGui::SameLine();
	ImGui::BeginGroup();
	if ((used_vs_now != imgui.model_vs_index)
		|| (used_ps_now != imgui.model_ps_index))
	{
		if (ImGui::Button("Apply changes"))
		{
			if (used_vs_now != imgui.model_vs_index)
			{
				active_model.vs_shader_ = &shaders.vs_shaders_[imgui.model_vs_index];
			}
			if (used_ps_now != imgui.model_ps_index)
			{
				active_model.ps_shader_ = &shaders.ps_shaders_[imgui.model_ps_index];
			}
		}
		if (ImGui::Button("Reset"))
		{
			imgui.model_vs_index = used_vs_now;
			imgui.model_ps_index = used_ps_now;
		}
	}
	ImGui::EndGroup();
}

void TickImGui(AppState& app)
{
	if (app.imgui_.show_demo_window)
	{
		ImGui::ShowDemoWindow(&app.imgui_.show_demo_window);
	}
	if (!app.imgui_.show)
	{
		return;
	}

	ImGui_TweaksInput(app.imgui_);
	ImGui_ShadersInput(app.imgui_);
}
