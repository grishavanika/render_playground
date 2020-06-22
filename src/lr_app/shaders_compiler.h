#pragma once
#include <Windows.h>
#include <span>
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <type_traits>

#pragma warning(push)
// macro redefinition
#pragma warning(disable : 4005)
#include <d3d11.h>
#pragma warning(pop)

#include "utils.h"
#include "read_directory_changes.h"

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

struct ShadersRef
{
    const ShaderInfo* vs_shader;
    const ShaderInfo* ps_shader;
    ShadersCompiler* compiler;
    ShadersWatch* watch;
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

struct ShadersWatch
{
    struct ShaderPatch
    {
        const ShaderInfo* shader_info;
        ComPtr<ID3D11VertexShader> vs_shader;
        ComPtr<ID3D11InputLayout> vs_layout;
        ComPtr<ID3D11PixelShader> ps_shader;
    };

    explicit ShadersWatch(ShadersCompiler& compiler);

    void recompile_on_change(const ShaderInfo& shader);
    ShaderPatch fetch_latest_version(const ShaderInfo& shader);

    void start_all();
    void collect_changes(ID3D11Device& device);

private:
    void add_watch(const ShaderInfo& shader, const wchar_t* file);
    void add_newest_patch(ShaderPatch patch);

private:
    template<typename T>
    using BufferFor = std::aligned_storage_t<sizeof(T), alignof(T)>;
    struct Directory;

    ShadersCompiler* compiler_;
    std::vector<std::unique_ptr<Directory>> dirs_;
    wi::IoCompletionPort io_port_;
    std::vector<ShaderPatch> patches_;

    struct Directory
    {
        struct FileMap
        {
            std::wstring file_name;
            std::vector<const ShaderInfo*> shaders;
        };

        std::wstring directory_path;
        std::vector<FileMap> files;

        wi::WinHANDLE directory = nullptr;
        DWORD buffer[16 * 1024];
        BufferFor<wi::DirectoryChanges> watcher_data;

        static std::unique_ptr<Directory> make(
            std::wstring directory_path
            , wi::IoCompletionPort& io_port
            , wi::WinULONG_PTR key);

        wi::DirectoryChanges& watcher() { return *reinterpret_cast<wi::DirectoryChanges*>(&watcher_data); }
        ~Directory();
        Directory(const Directory&) = delete;
        Directory(Directory&&) = delete;
        Directory& operator=(const Directory&) = delete;
        Directory& operator=(Directory&&) = delete;
    private:
        Directory() = default;
    };
};

