#include <windows.h>

#include <Windows.h>
#include <Windowsx.h>
#include <tchar.h>

#pragma warning(push)
// macro redefinition
#pragma warning(disable : 4005)
#include <d3d11.h>
#pragma warning(pop)

#pragma warning(push)
// conversion requires a narrowing conversion
#pragma warning(disable : 4838)
#include <xnamath.h>
#pragma warning(pop)

#include <cstdlib>

#include <vector>
#include <unordered_set>

#include "shaders/vs.h"
#include "shaders/ps.h"

#include "stub_window.h"
#include "utils.h"

#define XX_OBJECT_ROTATE() 0
#define XX_WIREFRAME() 0
#define XX_RENDER_2_OBJECTS() 1

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
};

D3D11_INPUT_ELEMENT_DESC layout[] =
{
    {
        .SemanticName = "position",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(Vertex, position),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
    {
        .SemanticName = "normal",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(Vertex, normal),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
};

// v -2.40469 2.71917 -2.89007
// vn 0.316163 -0.948077 0.034505
// f -2283//-2283 -2395//-2395 -2344//-2344 
void LoadObj_Simple(const char* filename
    , std::vector<Vertex>& verticies
    , std::vector<WORD>& indices)
{
    FILE* f = nullptr;
    const errno_t e = fopen_s(&f, filename, "rb");
    Panic(!e);

    char header[2];
    int vn_index = 0;

    auto add_index = [&](int v)
    {
        Panic(v < 0);
        const int verticies_count = int(verticies.size());
        int index = verticies_count + v;
        if ((index >= 0) && (index < verticies_count))
        {
            indices.push_back(WORD(index));
        }
        else
        {
            Panic(false);
        }
    };

    while (fread_s(header, 2, 1, 2, f) == 2)
    {
        if (strncmp(header, "v ", 2) == 0)
        {
            Vertex v;
            const int c = fscanf_s(f, "%f %f %f\n"
                , &v.position.x, &v.position.y, &v.position.z);
            Panic(c == 3);
            verticies.push_back(v);
        }
        else if (strncmp(header, "vn", 2) == 0)
        {
            Panic(vn_index < int(verticies.size()));
            Vertex& v = verticies[vn_index];
            const int c = fscanf_s(f, " %f %f %f\n"
                , &v.normal.x, &v.normal.y, &v.normal.z);
            Panic(c == 3);
            ++vn_index;
        }
        else if (strncmp(header, "f ", 2) == 0)
        {
            int v1 = 0;
            int v2 = 0;
            int v3 = 0;
            int _ = 0;
            const int c = fscanf_s(f, "%i//%i %i//%i %i//%i \n"
                , &v1, &_, &v2, &_, &v3, &_);
            Panic(c == 6);
            add_index(v1);
            add_index(v2);
            add_index(v3);
        }
        else
        {
            Panic(false && "Unknown .obj directive");
        }
    }

    Panic(vn_index == int(verticies.size()));

    fclose(f);
}
#if (0)
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

void LoadObj_Assimp(const char* filename
    , std::vector<Vertex>& verticies
    , std::vector<WORD>& indices
    , int meshIdx = 0)
{
    Assimp::Importer importer;
    const aiScene* scene =
        importer.ReadFile(filename, aiProcess_Triangulate);
    Panic(scene);
    Panic(meshIdx < int(scene->mNumMeshes));
    const aiMesh* mesh = scene->mMeshes[meshIdx];
    Panic(mesh);

    verticies.reserve(mesh->mNumVertices);
    for (unsigned i = 0; i < mesh->mNumVertices; ++i)
    {
        verticies.push_back({});
        Vertex& v = verticies.back();
        v.position.x = mesh->mVertices[i].x;
        v.position.y = mesh->mVertices[i].y;
        v.position.z = mesh->mVertices[i].z;
        if (mesh->mNormals)
        {
            v.normal.x = mesh->mNormals[i].x;
            v.normal.y = mesh->mNormals[i].y;
            v.normal.z = mesh->mNormals[i].z;
        }
    }
    for (int i = 0; i < int(mesh->mNumFaces); ++i)
    {
        for (int j = 0; j < int(mesh->mFaces[i].mNumIndices); ++j)
        {
            indices.push_back(WORD(mesh->mFaces[i].mIndices[j]));
        }
    }
}
#endif

void LoadObj_Predefined(
    std::vector<Vertex>& verticies
    , std::vector<WORD>& indices)
{
    const Vertex vertices_raw[] =
    {
        {XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f)},
        {XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f)},
        {XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f)},

        {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f)},
        {XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f)},

        {XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)},
        {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)},
        {XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f)},

        {XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
        {XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
        {XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)},

        {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f)},
        {XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f)},
        {XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f)},

        {XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
        {XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
        {XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
    };

    const WORD indices_raw[] =
    {
        3, 1, 0,
        2, 1, 3,

        6, 4, 5,
        7, 4, 6,

        11, 9, 8,
        10, 9, 11,

        14, 12, 13,
        15, 12, 14,

        19, 17, 16,
        18, 17, 19,

        22, 20, 21,
        23, 20, 22
    };

    verticies.assign(vertices_raw, vertices_raw + 24);
    indices.assign(indices_raw, indices_raw + 36);
}

#pragma warning(push)
// structure was padded due to alignment specifier
#pragma warning(disable:4324)
struct GameState
{
    float fov_y_ = DegreesToRadians(45.f);
    float aspect_ratio_ = 0.f;
    float mouse_scroll_sensitivity_ = 0.05f;
    float camera_yaw_degrees_ = 0.f;
    float camera_pitch_degrees_ = 0.f;
    float camera_rotation_mouse_sensitivity_ = 0.05f;
    float camera_move_speed_ = 0.5f;

    std::unordered_set<WPARAM> keys_down_;

    XMVECTOR camera_position_     = XMVectorSet(0.0f, 0.0f, -15.0f, 0.0f);
    const XMVECTOR camera_up_dir_ = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR camera_front_dir_    = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMVECTOR camera_right_dir_    = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    D3D11_VIEWPORT vp_{};
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* device_context_ = nullptr;
    IDXGISwapChain* swap_chain_ = nullptr;
    ID3D11RenderTargetView* render_target_view_ = nullptr;
    ID3D11DepthStencilView* depth_buffer_ = nullptr;
};
#pragma warning(pop)

int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
    const char* const obj = R"(D:\Downloads\skull_part.obj)";
    //const char* const obj = R"(skull_part.obj)";
    //const char* const obj = R"(K:\teapot.obj.txt)";

    std::vector<Vertex> obj_verticies;
    std::vector<WORD> obj_indices;
#if (1)
    LoadObj_Simple(obj, obj_verticies, obj_indices);
#elif (0)
    LoadObj_Assimp(obj, obj_verticies, obj_indices);
#else
    LoadObj_Predefined(obj_verticies, obj_indices);
    (void)obj;
#endif

    Panic(!obj_verticies.empty());
    Panic(!obj_indices.empty());

    GameState game;
    StubWindow window("_xxxWindowClassName");
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
        game.render_target_view_->Release();
        game.render_target_view_ = nullptr;
        // Preserve the existing buffer count and format.
        HRESULT hr = game.swap_chain_->ResizeBuffers(0, UINT(width), UINT(height), DXGI_FORMAT_UNKNOWN, 0);
        Panic(SUCCEEDED(hr));

        // Get buffer and create a render-target-view.
        ID3D11Texture2D* buffer = nullptr;
        hr = game.swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buffer);
        Panic(SUCCEEDED(hr));

        hr = game.device_->CreateRenderTargetView(buffer , nullptr, &game.render_target_view_);
        Panic(SUCCEEDED(hr));
        buffer->Release();
        buffer = nullptr;

        game.depth_buffer_->Release();
        game.depth_buffer_ = nullptr;

        D3D11_TEXTURE2D_DESC depthTextureDesc{};
        depthTextureDesc.Width = UINT(width);
        depthTextureDesc.Height = UINT(height);
        depthTextureDesc.MipLevels = 1;
        depthTextureDesc.ArraySize = 1;
        depthTextureDesc.SampleDesc.Count = 1;
        depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ID3D11Texture2D* depth_stencil_texture = nullptr;
        hr = game.device_->CreateTexture2D(&depthTextureDesc, nullptr, &depth_stencil_texture);
        Panic(SUCCEEDED(hr));

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = depthTextureDesc.Format;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

        hr = game.device_->CreateDepthStencilView(depth_stencil_texture, &dsvDesc, &game.depth_buffer_);
        depth_stencil_texture->Release();
        Panic(SUCCEEDED(hr));

        game.device_context_->OMSetRenderTargets(1, &game.render_target_view_, game.depth_buffer_);

        // Set up the viewport.
        game.vp_.Width = static_cast<FLOAT>(width);
        game.vp_.Height = static_cast<FLOAT>(height);

        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
    window.on_message(WM_MOUSEWHEEL
        , [&game](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        const float delta_wheel = GET_WHEEL_DELTA_WPARAM(wparam);
        const float dv = (delta_wheel / WHEEL_DELTA) * game.mouse_scroll_sensitivity_;
        game.fov_y_ = std::clamp(game.fov_y_ - dv, DegreesToRadians(1.f), DegreesToRadians(90.f));
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });

    RAWINPUTDEVICE Rid{};
    Rid.usUsagePage = 0x01; // generic
    Rid.usUsage = 0x02; // mouse
    Rid.dwFlags = RIDEV_INPUTSINK;
    Rid.hwndTarget = window.wnd();
    Panic(::RegisterRawInputDevices(&Rid, 1, sizeof(RAWINPUTDEVICE)));
    window.on_message(WM_INPUT
        , [&game](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
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

        const float d_yaw = (x_delta * game.camera_rotation_mouse_sensitivity_);
        const float d_pitch = (y_delta * game.camera_rotation_mouse_sensitivity_);

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
        (void)game.keys_down_.erase(wparam);
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });
    window.on_message(WM_KEYDOWN
        , [&game](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        (void)game.keys_down_.insert(wparam);
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    });

    Panic(::ShowWindow(window.wnd(), SW_SHOW) == 0/*was previously hidden*/);

    RECT client_rect{};
    Panic(!!::GetClientRect(window.wnd(), &client_rect));
    const UINT client_width = (client_rect.right - client_rect.left);
    const UINT client_height = (client_rect.bottom - client_rect.top);
    game.aspect_ratio_ = (client_width / static_cast<FLOAT>(client_height));
    
    { // Calculate default camera orientation from initial angles.
        const float yaw = DegreesToRadians(game.camera_yaw_degrees_);
        const float pitch = DegreesToRadians(game.camera_pitch_degrees_);

        const float x = cosf(yaw) * cosf(pitch);
        const float y = sinf(pitch);
        const float z = sinf(yaw) * cosf(DegreesToRadians(game.camera_pitch_degrees_));

        game.camera_front_dir_ = XMVector3Normalize(XMVectorSet(x, y, z, 0.f));
        game.camera_right_dir_ = XMVector3Normalize(XMVector3Cross(game.camera_front_dir_, game.camera_up_dir_));
    }

    // Device initialization
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

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = game.swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&pBackBuffer));
    Panic(SUCCEEDED(hr));
    hr = game.device_->CreateRenderTargetView(pBackBuffer, nullptr, &game.render_target_view_);
    Panic(SUCCEEDED(hr));
    pBackBuffer->Release();

    // Setup the viewport
    game.vp_.Width = static_cast<FLOAT>(client_width);
    game.vp_.Height = static_cast<FLOAT>(client_height);
    game.vp_.MinDepth = 0.0f;
    game.vp_.MaxDepth = 1.0f;
    game.vp_.TopLeftX = 0;
    game.vp_.TopLeftY = 0;

    // VS & IA
    ID3D11VertexShader* vertexShader = nullptr;
    hr = game.device_->CreateVertexShader(k_VS, sizeof(k_VS), nullptr, &vertexShader);
    Panic(SUCCEEDED(hr));

    ID3D11InputLayout* vertexLayout = nullptr;
    hr = game.device_->CreateInputLayout(layout, _countof(layout), k_VS, sizeof(k_VS), &vertexLayout);
    Panic(SUCCEEDED(hr));

    // PS
    ID3D11PixelShader* pixelShader = nullptr;
    hr = game.device_->CreatePixelShader(k_PS, sizeof(k_PS), nullptr, &pixelShader);
    Panic(SUCCEEDED(hr));

    // VB
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = UINT(obj_verticies.size() * sizeof(Vertex));
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData{};
    InitData.pSysMem = obj_verticies.data();
    ID3D11Buffer* vertexBuffer = nullptr;
    hr = game.device_->CreateBuffer(&bd, &InitData, &vertexBuffer);
    Panic(SUCCEEDED(hr));

    // Create index buffer
    ID3D11Buffer* indexBuffer = nullptr;

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = UINT(obj_indices.size() * sizeof(WORD));
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = obj_indices.data();
    hr = game.device_->CreateBuffer(&bd, &InitData, &indexBuffer);
    Panic(SUCCEEDED(hr));

    struct ConstantBuffer
    {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
    };

    // Create the constant buffer
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    ID3D11Buffer* constantBuffer = nullptr;
    hr = game.device_->CreateBuffer(&bd, nullptr, &constantBuffer);
    Panic(SUCCEEDED(hr));

    D3D11_TEXTURE2D_DESC depthTextureDesc{};
    depthTextureDesc.Width = client_width;
    depthTextureDesc.Height = client_height;
    depthTextureDesc.MipLevels = 1;
    depthTextureDesc.ArraySize = 1;
    depthTextureDesc.SampleDesc.Count = 1;
    depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depth_stencil_texture = nullptr;
    hr = game.device_->CreateTexture2D(&depthTextureDesc, nullptr, &depth_stencil_texture);
    Panic(SUCCEEDED(hr));

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = depthTextureDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

    hr = game.device_->CreateDepthStencilView(depth_stencil_texture, &dsvDesc, &game.depth_buffer_);
    depth_stencil_texture->Release();
    Panic(SUCCEEDED(hr));

    D3D11_RASTERIZER_DESC wfd{};
#if (XX_WIREFRAME())
    wfd.FillMode = D3D11_FILL_WIREFRAME;
#else
    wfd.FillMode = D3D11_FILL_SOLID;
#endif
    wfd.CullMode = D3D11_CULL_NONE;
    wfd.DepthClipEnable = TRUE;
    wfd.AntialiasedLineEnable = TRUE;
    ID3D11RasterizerState* rasterizerState = nullptr;
    hr = game.device_->CreateRasterizerState(&wfd, &rasterizerState);
    Panic(SUCCEEDED(hr));

    const float k_ClearColor[4] = {0.f, 0.f, 0.0f, 1.0f};
    
    // Initialize the view matrix.
    XMMATRIX projection = XMMatrixIdentity();
    XMMATRIX world = XMMatrixIdentity();
    XMMATRIX view = XMMatrixIdentity();

    const DWORD dwTimeStart = ::GetTickCount();

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

        projection = XMMatrixPerspectiveFovLH(game.fov_y_
            , game.aspect_ratio_
            , 0.01f // NearZ
            , 100.0f); // FarZ
        const float t = (::GetTickCount() - dwTimeStart) / 1000.0f;

        view = XMMatrixLookAtLH(game.camera_position_
            , game.camera_position_ + game.camera_front_dir_
            , game.camera_up_dir_);

        const float scale = 2.f;
        world = XMMatrixScaling(scale, scale, scale)
            * XMMatrixRotationY(2.5);

#if (XX_OBJECT_ROTATE())
        world = (world * XMMatrixRotationY(t));
#endif
#if (1) // translate
        world = (world * XMMatrixTranslation(+40.f, -10.f, +5.f));
#endif

        // Just clear the backbuffer
        game.device_context_->ClearRenderTargetView(game.render_target_view_, k_ClearColor);
        game.device_context_->ClearDepthStencilView(game.depth_buffer_
            , D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        {
            ConstantBuffer cb;
            cb.world = XMMatrixTranspose(world);
            cb.view = XMMatrixTranspose(view);
            cb.projection = XMMatrixTranspose(projection);

            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            // Input Assembler.
            game.device_context_->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
            game.device_context_->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
            game.device_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            game.device_context_->IASetInputLayout(vertexLayout);
            // Vertex Shader.
            game.device_context_->VSSetShader(vertexShader, nullptr, 0);
            game.device_context_->UpdateSubresource(constantBuffer, 0, nullptr, &cb, 0, 0);
            game.device_context_->VSSetConstantBuffers(0, 1, &constantBuffer);
            // Rasterizer Stage.
            game.device_context_->RSSetState(rasterizerState);
            game.device_context_->RSSetViewports(1, &game.vp_);
            // Pixel Shader.
            game.device_context_->PSSetShader(pixelShader, nullptr, 0);
            // Output Merger.
            game.device_context_->OMSetRenderTargets(1, &game.render_target_view_, game.depth_buffer_);

            // Actual draw call.
            game.device_context_->DrawIndexed(UINT(obj_indices.size()), 0, 0);
        }

#if (XX_RENDER_2_OBJECTS())
        {
            XMMATRIX world2 = world * XMMatrixTranslation(-15.f, +7.f, +7.f);

            ConstantBuffer cb;
            cb.world = XMMatrixTranspose(world2);
            cb.view = XMMatrixTranspose(view);
            cb.projection = XMMatrixTranspose(projection);

            // Update constants for Vertex shader.
            game.device_context_->UpdateSubresource(constantBuffer, 0, nullptr, &cb, 0, 0);

            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            // Input Assembler.
            game.device_context_->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
            game.device_context_->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
            game.device_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            game.device_context_->IASetInputLayout(vertexLayout);
            // Vertex Shader.
            game.device_context_->VSSetShader(vertexShader, nullptr, 0);
            game.device_context_->UpdateSubresource(constantBuffer, 0, nullptr, &cb, 0, 0);
            game.device_context_->VSSetConstantBuffers(0, 1, &constantBuffer);
            // Rasterizer Stage.
            game.device_context_->RSSetState(rasterizerState);
            game.device_context_->RSSetViewports(1, &game.vp_);
            // Pixel Shader.
            game.device_context_->PSSetShader(pixelShader, nullptr, 0);
            // Output Merger.
            game.device_context_->OMSetRenderTargets(1, &game.render_target_view_, game.depth_buffer_);

            // Actual draw call.
            game.device_context_->DrawIndexed(UINT(obj_indices.size()), 0, 0);
        }
#endif
        const UINT SyncInterval = 1; // Synchronize presentation after single vertical blank.
        const HRESULT ok = game.swap_chain_->Present(SyncInterval, 0);
        Panic(SUCCEEDED(ok));
    }

    game.device_context_->ClearState();
    game.render_target_view_->Release();
    game.swap_chain_->Release();
    game.device_context_->Release();
    game.device_->Release();
    vertexBuffer->Release();
    vertexLayout->Release();
    vertexShader->Release();
    pixelShader->Release();
    constantBuffer->Release();
    indexBuffer->Release();
    rasterizerState->Release();
    game.depth_buffer_->Release();

    return 0;
}
