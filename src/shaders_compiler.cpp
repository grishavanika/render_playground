#include "shaders_compiler.h"
#include <filesystem>
#include <algorithm>

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

/*explicit*/ ShadersWatch::ShadersWatch(ShadersCompiler& compiler)
    : compiler_(&compiler)
    , dirs_()
    , io_port_()
{
    std::error_code ec;
    auto port = wi::IoCompletionPort::make(1, ec);
    Panic(!!port);
    Panic(!ec);
    io_port_ = std::move(*port);
}

void ShadersWatch::watch_changes_to(const ShaderInfo& shader, const void* user_data)
{
    add_watch(shader, shader.file_name, user_data);
    for (const ShaderInfo::Dependency& dep : shader.dependencies)
    {
        add_watch(shader, dep.file_name, user_data);
    }
}

void ShadersWatch::add_watch(const ShaderInfo& shader, const wchar_t* file, const void* user_data)
{
    namespace fs = std::filesystem;
    fs::path path = fs::canonical(file);

    auto dir_path = path.parent_path().wstring();
    Directory* dir = nullptr;
    for (std::unique_ptr<Directory>& d : dirs_)
    {
        if (d->directory_path == dir_path)
        {
            dir = d.get();
            break;
        }
    }
    if (!dir)
    {
        const wi::WinULONG_PTR key = wi::WinULONG_PTR(dirs_.size());
        dir = dirs_.emplace_back(Directory::make(std::move(dir_path), io_port_, key)).get();
        std::error_code ec;
        dir->watcher().start_watch(ec);
        Panic(!ec);
    }
    Panic(dir);

    auto file_name = path.filename().wstring();
    FileShadersDeps* file_deps = nullptr;
    for (FileShadersDeps& deps : dir->files)
    {
        if (deps.file_name != file_name)
        {
            continue;
        }
        for (const ShaderState& state : deps.shaders)
        {
            if (state.info_ == &shader)
            {
                return;
            }
        }
        file_deps = &deps;
        break;
    }
    if (!file_deps)
    {
        FileShadersDeps deps;
        deps.file_name = std::move(file_name);
        file_deps = &dir->files.emplace_back(std::move(deps));
    }
    ShaderState state{.info_ = &shader, .user_data_ = user_data};
    file_deps->shaders.push_back(state);
}

/*static*/ auto ShadersWatch::Directory::make(
    std::wstring directory_path
    , wi::IoCompletionPort& io_port
    , wi::WinULONG_PTR key)
        -> std::unique_ptr<Directory>
{
    std::unique_ptr<Directory> ptr(new Directory());
    ptr->directory_path = std::move(directory_path);
    void* mem = static_cast<void*>(&ptr->watcher_data);
    std::error_code ec;
    auto dir_changes = wi::DirectoryChanges::make(
        ptr->directory_path.c_str()
        , ptr->buffer
        , sizeof(ptr->buffer)
        , false // do NOT watch subtree
        , FILE_NOTIFY_CHANGE_LAST_WRITE
        , io_port
        , key
        , ec);
    Panic(!ec);
    Panic(!!dir_changes);
    (void)new(mem) wi::DirectoryChanges(std::move(*dir_changes));
    return ptr;
}

ShadersWatch::Directory::~Directory()
{
    using DirectoryChanges = wi::DirectoryChanges;
    watcher().~DirectoryChanges();
}

std::vector<ShaderPatch> ShadersWatch::collect_changes(ID3D11Device& device)
{
    std::vector<ShaderPatch> patches;

    auto add_newest_patch = [&](ShaderPatch new_patch)
    {
        auto it = std::remove_if(patches.begin(), patches.end()
            , [&new_patch](const ShaderPatch& existing)
        {
            return (existing.shader_info == new_patch.shader_info);
        });
        patches.erase(it, patches.end());
        patches.push_back(std::move(new_patch));
    };

    std::vector<ShaderState> shaders;
    std::error_code ec;
    while (std::optional<wi::PortEntry> data = io_port_.query(ec))
    {
        Panic(!ec);
        if (!data)
        {
            continue;
        }
        if (data->completion_key >= wi::WinULONG_PTR(dirs_.size()))
        {
            continue;
        }
        Directory& dir = *dirs_[std::size_t(data->completion_key)];
        wi::DirectoryChangesRange changes(dir.buffer, *data);
        for (const wi::DirectoryChange& file_change : changes)
        {
            for (const FileShadersDeps& deps : dir.files)
            {
                if (deps.file_name == file_change.name)
                {
                    shaders.insert(shaders.end(), deps.shaders.begin(), deps.shaders.end());
                    break;
                }
            }
        }
        dir.watcher().start_watch(ec);
        Panic(!ec);
    }

    int count = 0;
    std::sort(shaders.begin(), shaders.end()
        , [](const ShaderState& lhs, const ShaderState& rhs)
    {
        return (lhs.info_ < rhs.info_);
    });
    auto unique_end = std::unique(shaders.begin(), shaders.end()
        , [](const ShaderState& lhs, const ShaderState& rhs)
    {
        return (lhs.info_ == rhs.info_);
    });
    for (auto it = shaders.begin(); it != unique_end; ++it)
    {
        const ShaderState& shader = *it;
        std::string errors;
        ComPtr<ID3DBlob> bytecode = compiler_->compile(*shader.info_, errors);
        if (!bytecode)
        {
            continue;
        }
        ShaderPatch patch;
        patch.shader_info = shader.info_;
        patch.user_data = shader.user_data_;
        switch (shader.info_->kind)
        {
        case ShaderInfo::VS:
        {
            compiler_->create_vs(device
                , *shader.info_
                , patch.vs_shader
                , patch.vs_layout
                , bytecode.Get());
            break;
        }
        case ShaderInfo::PS:
        {
            compiler_->create_ps(device
                , *shader.info_
                , patch.ps_shader
                , bytecode.Get());
            break;
        }
        }
        ++count;
        add_newest_patch(std::move(patch));
    }
    return patches;
}

