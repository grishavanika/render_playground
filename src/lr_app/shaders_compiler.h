#include <Windows.h>
#include <span>
#include <string>

#pragma warning(push)
// macro redefinition
#pragma warning(disable : 4005)
#include <d3d11.h>
#pragma warning(pop)

#include "utils.h"

struct ShaderInfo
{
    struct Dependency
    {
        const char* file_name;
    };
    struct Define
    {
        const char* name;
        const char* definition;
    };

    const char* debug_name;
    std::span<const BYTE> bytecode;
    const wchar_t* file_name;
    std::span<const D3D11_INPUT_ELEMENT_DESC> vs_layout;
    const char* entry_point_name;
    const char* profile;
    std::span<const Dependency> dependencies;
    std::span<const Define> defines;
};

struct ShadersCompiler;

struct ShadersRef
{
    const ShaderInfo* vs_shader;
    const ShaderInfo* ps_shader;
    ShadersCompiler* compiler;
};

struct ShadersCompiler
{
    void create_vs(ID3D11Device& device
        , const ShaderInfo& vs
        , ComPtr<ID3D11VertexShader>& vs_shader
        , ComPtr<ID3D11InputLayout>& vs_layout
        , ID3DBlob* bytecode = nullptr);

    void create_ps(ID3D11Device& device
        , const ShaderInfo& ps
        , ComPtr<ID3D11PixelShader>& ps_shader
        , ID3DBlob* bytecode = nullptr);

    ComPtr<ID3DBlob> compile(const ShaderInfo& shader_info
        , std::string& error_text);
};
