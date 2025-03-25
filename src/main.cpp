#include "utils.h"
#include "app_state.h"
#include "render_lines.h"
#include "render_model.h"
#include "render_vertices_only.h"
#include "render_with_normals.h"
#include "shaders_database.h"
#include "predefined_objects.h"

// Integration of ImGui comes from
// imgui-src/examples/example_win32_directx11/main.cpp
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/geometric.hpp>
#include <glm/ext.hpp>

#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>

#include <vector>
#include <unordered_set>
#include <filesystem>

#include <cstdlib>

#if !defined(XX_PACKAGE_FOLDER)
#  error "Build system missed to specify where package (binaries/data) is."
#endif
#if !defined(XX_SHADERS_FOLDER)
#  error "Build system missed to specify where shaders are."
#endif

int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	AppState app;
	app.window_ = StubWindow("xxx_lr");
	app.watch_ = ShadersWatch(app.compiler_);
	app.all_shaders_ = Shaders::Build();
	app.imgui_.app_ = &app;
	app.files_to_load_.push_back(XX_PACKAGE_FOLDER "dragon/dragon.obj");

	// Accept WM_DROPFILES.
	::DragAcceptFiles(app.window_.wnd(), TRUE);
	// Accept raw WM_INPUT.
	RAWINPUTDEVICE raw_mouse{};
	raw_mouse.usUsagePage = 0x01; // generic
	raw_mouse.usUsage = 0x02; // mouse
	raw_mouse.dwFlags = RIDEV_INPUTSINK;
	raw_mouse.hwndTarget = app.window_.wnd();
	Panic(::RegisterRawInputDevices(&raw_mouse, 1, sizeof(RAWINPUTDEVICE)));

	Init_MessageHandling(app);

	Panic(::ShowWindow(app.window_.wnd(), SW_SHOW) == 0/*was previously hidden*/);

	RECT client_rect{};
	Panic(!!::GetClientRect(app.window_.wnd(), &client_rect));
	const UINT client_width = (client_rect.right - client_rect.left);
	const UINT client_height = (client_rect.bottom - client_rect.top);
	app.window_width_ = float(client_width);
	app.window_height_ = float(client_height);

	{ // Calculate default camera orientation from initial angles.
		const float x = cosf(app.camera_yaw_) * cosf(app.camera_pitch_);
		const float y = sinf(app.camera_pitch_);
		const float z = sinf(app.camera_yaw_) * cosf(app.camera_pitch_);

		app.camera_front_dir_ = glm::normalize(glm::vec3(x, y, z));
		app.camera_right_dir_ = glm::normalize(glm::cross(app.camera_front_dir_, app.camera_up_dir_));
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
	sc_desc.OutputWindow = app.window_.wnd();
	sc_desc.SampleDesc.Count = 4;
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
		, &app.swap_chain_
		, &app.device_
		, &unused
		, &app.device_context_);
	Panic(SUCCEEDED(hr));

	// Create a render target view.
	ComPtr<ID3D11Texture2D> back_buffer;
	hr = app.swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &back_buffer);
	Panic(SUCCEEDED(hr));
	hr = app.device_->CreateRenderTargetView(back_buffer.Get(), nullptr, &app.render_target_view_);
	Panic(SUCCEEDED(hr));
	back_buffer.Reset();

	// Setup the viewport.
	app.vp_.Width = static_cast<FLOAT>(client_width);
	app.vp_.Height = static_cast<FLOAT>(client_height);
	app.vp_.MinDepth = 0.0f;
	app.vp_.MaxDepth = 1.0f;
	app.vp_.TopLeftX = 0;
	app.vp_.TopLeftY = 0;

	// Z-test/buffer.
	D3D11_TEXTURE2D_DESC dept_texture_desc{};
	dept_texture_desc.Width = client_width;
	dept_texture_desc.Height = client_height;
	dept_texture_desc.MipLevels = 1;
	dept_texture_desc.ArraySize = 1;
	dept_texture_desc.SampleDesc.Count = 4;
	dept_texture_desc.SampleDesc.Quality = 0;
	dept_texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dept_texture_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ComPtr<ID3D11Texture2D> depth_stencil_texture;
	hr = app.device_->CreateTexture2D(&dept_texture_desc, nullptr, &depth_stencil_texture);
	Panic(SUCCEEDED(hr));

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = dept_texture_desc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

	hr = app.device_->CreateDepthStencilView(depth_stencil_texture.Get(), &dsvDesc, &app.depth_buffer_);
	depth_stencil_texture.Reset();
	Panic(SUCCEEDED(hr));

	// Ability to enable/disable wireframe.
	D3D11_RASTERIZER_DESC wfd{};
	wfd.FillMode = D3D11_FILL_SOLID;
	wfd.CullMode = D3D11_CULL_NONE;
	wfd.DepthClipEnable = TRUE;
	wfd.AntialiasedLineEnable = FALSE;
	wfd.MultisampleEnable = TRUE;
	ComPtr<ID3D11RasterizerState> rasterizer_state;
	hr = app.device_->CreateRasterizerState(&wfd, &rasterizer_state);
	Panic(SUCCEEDED(hr));

	ImGui_Setup(app);

	RenderLines render_lines = RenderLines::make(app.device_);
	render_lines.vs_shader_ = app.all_shaders_.find_vs(c_vs_lines);
	render_lines.ps_shader_ = app.all_shaders_.find_ps(c_ps_lines);
	RenderLines render_bb = RenderLines::make(app.device_);
	render_bb.vs_shader_ = app.all_shaders_.find_vs(c_vs_lines);
	render_bb.ps_shader_ = app.all_shaders_.find_ps(c_ps_lines);
	RenderVertices render_light_cube = make_cube_vertices_only(app.device_);
	render_light_cube.vs_shader_ = app.all_shaders_.find_vs(c_vs_vertices_only);
	render_light_cube.ps_shader_ = app.all_shaders_.find_ps(c_ps_vertices_only);

	Init_KnownShaders(app);

	render_lines.add_bb(glm::vec3(-1.f), glm::vec3(1.f)
		, glm::vec3(1.f, 0.f, 0.f));
	// Positive World X direction. RED.
	render_lines.add_line(
		glm::vec3(0.f, 0.f, 0.f)
		, glm::vec3(1.f, 0.f, 0.f)
		, glm::vec3(1.f, 0.f, 0.f));
	// Positive World Y direction. GREEN.
	render_lines.add_line(
		glm::vec3(0.f, 0.f, 0.f)
		, glm::vec3(0.f, 1.f, 0.f)
		, glm::vec3(0.f, 1.f, 0.f));
	// Positive World Z direction. BLUE.
	render_lines.add_line(
		glm::vec3(0.f, 0.f, 0.f)
		, glm::vec3(0.f, 0.f, 1.f)
		, glm::vec3(0.f, 0.f, 1.f));

	// Initialize the view matrix.
	glm::mat4x4 projection = glm::mat4x4(1.f);
	glm::mat4x4 view = glm::mat4x4(1.f);

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

		if (TickModelsLoad(app))
		{
			const Model& m = app.models_[std::size_t(app.active_model_index_)].model;
			render_bb.clear();
			render_bb.add_bb(m.aabb_min(), m.aabb_max(), glm::vec3(1.f, 0.f, 0.f));
		}

		TickShadersChange(app);

		// Start the Dear ImGui frame.
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		TickImGui(app);
		TickInput(app);

		projection = glm::perspectiveFovLH(app.fov_y_
			, app.window_width_
			, app.window_height_
			, 0.01f    // NearZ
			, 10000.0f); // FarZ
		const float t = float(::GetTickCount() - start_time) / 1000.f;

		view = glm::lookAtLH(app.camera_position_
			, app.camera_position_ + app.camera_front_dir_
			, app.camera_up_dir_);

		app.active_model_.world = glm::mat4x4(1.f)
			* app.imgui_.get_model_scale()
			* app.model_rotation_;
		render_bb.world = app.active_model_.world;
		app.active_model_.light_color = app.imgui_.light_color;
		app.active_model_.viewer_position = app.camera_position_;

		switch (app.imgui_.light_mode)
		{
		case LightMode::Moving_Active:
		{
			const float cam_x = (sinf(t) * app.imgui_.light_move_radius);
			const float cam_z = (cosf(t) * app.imgui_.light_move_radius);
			app.active_model_.light_position = glm::vec3(cam_x, 0.0f, cam_z);
			break;
		}
		case LightMode::Moving_Paused:
			break;
		case LightMode::Static_AtCameraPosition:
			app.active_model_.light_position = app.camera_position_;
			break;
		}

		if (app.imgui_.check_wireframe_change())
		{
			wfd.FillMode = app.imgui_.wireframe
				? D3D11_FILL_WIREFRAME
				: D3D11_FILL_SOLID;
			rasterizer_state.Reset();
			hr = app.device_->CreateRasterizerState(&wfd, &rasterizer_state);
			Panic(SUCCEEDED(hr));
		}

		// Clear.
		const float c_clear_color[4] = { 1.f, 1.f, 1.0f, 1.0f };
		app.device_context_->ClearRenderTargetView(app.render_target_view_.Get(), c_clear_color);
		app.device_context_->ClearDepthStencilView(app.depth_buffer_.Get()
			, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		// Output Merger.
		app.device_context_->OMSetRenderTargets(1, app.render_target_view_.GetAddressOf(), app.depth_buffer_.Get());
		// Rasterizer Stage.
		app.device_context_->RSSetState(rasterizer_state.Get());
		app.device_context_->RSSetViewports(1, &app.vp_);

		if (app.imgui_.show_light_cube)
		{
			render_light_cube.world = glm::translate(glm::mat4x4(1.f), app.active_model_.light_position);
			render_light_cube.render(*app.device_context_.Get(), view, projection);
		}
		if (app.imgui_.show_zero_world_space)
		{
			render_lines.render(*app.device_context_.Get(), view, projection);
		}
		if (app.imgui_.show_model)
		{
			app.active_model_.render(*app.device_context_.Get(), view, projection);
			render_bb.render(*app.device_context_.Get(), view, projection);
		}

		// Rendering
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present.
		const UINT SyncInterval = 1; // Synchronize presentation after single vertical blank.
		hr = app.swap_chain_->Present(SyncInterval, 0);
		Panic(SUCCEEDED(hr));
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	return 0;
}
