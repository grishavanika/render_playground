#include "shaders_compiler.h"

#include <d3dcompiler.h>

void ShadersCompiler::create_vs(ID3D11Device& device
    , const ShaderInfo& vs
    , ComPtr<ID3D11VertexShader>& vs_shader
    , ComPtr<ID3D11InputLayout>& vs_layout
    , ID3DBlob* bytecode /*= nullptr*/)
{
    Panic(!vs.bytecode.empty());
    Panic(!vs.vs_layout.empty());

    const void* bytecode_data = vs.bytecode.data();
    SIZE_T bytecode_size = vs.bytecode.size();
    if (bytecode)
    {
        bytecode_data = bytecode->GetBufferPointer();
        bytecode_size = bytecode->GetBufferSize();
    }

    HRESULT hr = device.CreateVertexShader(
        bytecode_data
        , bytecode_size
        , nullptr
        , &vs_shader);
    Panic(SUCCEEDED(hr));

    hr = device.CreateInputLayout(
        vs.vs_layout.data()
        , UINT(vs.vs_layout.size())
        , bytecode_data
        , bytecode_size
        , &vs_layout);
    Panic(SUCCEEDED(hr));
}

void ShadersCompiler::create_ps(ID3D11Device& device
    , const ShaderInfo& ps
    , ComPtr<ID3D11PixelShader>& ps_shader
    , ID3DBlob* bytecode /*= nullptr*/)
{
    Panic(!ps.bytecode.empty());

    const void* bytecode_data = ps.bytecode.data();
    SIZE_T bytecode_size = ps.bytecode.size();
    if (bytecode)
    {
        bytecode_data = bytecode->GetBufferPointer();
        bytecode_size = bytecode->GetBufferSize();
    }

    HRESULT hr = device.CreatePixelShader(
        bytecode_data
        , bytecode_size
        , nullptr
        , &ps_shader);
    Panic(SUCCEEDED(hr));
}

ComPtr<ID3DBlob> ShadersCompiler::compile(const ShaderInfo& shader_info
    , std::string& error_text)
{
    // https://docs.microsoft.com/en-us/windows/win32/direct3d11/how-to--compile-a-shader
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG;
#endif

    const D3D_SHADER_MACRO* defines = nullptr;

    ComPtr<ID3DBlob> shader_blob;
    ComPtr<ID3DBlob> error_blob;
    const HRESULT hr = ::D3DCompileFromFile(
        shader_info.file_name
        , defines
        , D3D_COMPILE_STANDARD_FILE_INCLUDE
        , shader_info.entry_point_name
        , shader_info.profile
        , flags
        , 0
        , &shader_blob
        , &error_blob);
    if (SUCCEEDED(hr))
    {
        return shader_blob;
    }

    if (error_blob)
    {
        const char* const str = static_cast<const char*>(error_blob->GetBufferPointer());
        const std::size_t length = error_blob->GetBufferSize();
        error_text.assign(str, length);
    }
    return ComPtr<ID3DBlob>();
}
