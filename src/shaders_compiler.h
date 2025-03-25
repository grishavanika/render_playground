#pragma once
#include <span>
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <type_traits>

#include "dx_api.h"

#include "utils.h"

#include <read_directory_changes.h>

struct ShaderInfo
{
    enum Kind { VS, PS };

    struct Dependency
    {
        const wchar_t* file_name;
    };
    struct Define
    {
        const char* name;
        const char* definition;
    };

    const char* debug_name;
    Kind kind;
    std::span<const BYTE> bytecode;
    const wchar_t* file_name;
    std::span<const D3D11_INPUT_ELEMENT_DESC> vs_layout;
    const char* entry_point_name;
    const char* profile;
    std::span<const Dependency> dependencies;
    std::span<const Define> defines;
};

struct ShadersCompiler;
struct ShadersWatch;

struct VSShader
{
    const ShaderInfo* vs_info;
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11InputLayout> vs_layout;
};

struct PSShader
{
    const ShaderInfo* ps_info;
    ComPtr<ID3D11PixelShader> ps;
};

static void PanicShadersValid(const VSShader* vs_shader, const PSShader* ps_shader)
{
    Panic(!!vs_shader);
    Panic(!!vs_shader->vs_layout);
    Panic(!!vs_shader->vs);
    Panic(!!ps_shader);
    Panic(!!ps_shader->ps);
}

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

struct ShaderPatch
{
    const ShaderInfo* shader_info;
    const void* user_data;
    ComPtr<ID3D11VertexShader> vs_shader;
    ComPtr<ID3D11InputLayout> vs_layout;
    ComPtr<ID3D11PixelShader> ps_shader;
};

struct ShadersWatch
{
    explicit ShadersWatch() = default;
    explicit ShadersWatch(ShadersCompiler& compiler);

    void watch_changes_to(const ShaderInfo& shader, const void* user_data);

    std::vector<ShaderPatch> collect_changes(ID3D11Device& device);

private:
    void add_watch(const ShaderInfo& shader, const wchar_t* file, const void* user_data);

private:
    template<typename T>
    using BufferFor = std::aligned_storage_t<sizeof(T), alignof(T)>;
    struct Directory;

    ShadersCompiler* compiler_ = nullptr;
    std::vector<std::unique_ptr<Directory>> dirs_;
    wi::IoCompletionPort io_port_;

    struct ShaderState
    {
        const ShaderInfo* info_;
        const void* user_data_;
    };

    struct FileShadersDeps
    {
        std::wstring file_name;
        std::vector<ShaderState> shaders;
    };

    struct Directory
    {
        std::wstring directory_path;
        std::vector<FileShadersDeps> files;

        DWORD buffer[1024];
        BufferFor<wi::DirectoryChanges> watcher_data;

        static std::unique_ptr<Directory> make(
            std::wstring directory_path
            , wi::IoCompletionPort& io_port
            , wi::WinULONG_PTR key);

        wi::DirectoryChanges& watcher()
        { return *reinterpret_cast<wi::DirectoryChanges*>(&watcher_data); }

        ~Directory();
        Directory(const Directory&) = delete;
        Directory(Directory&&) = delete;
        Directory& operator=(const Directory&) = delete;
        Directory& operator=(Directory&&) = delete;
    private:
        Directory() = default;
    };
};

