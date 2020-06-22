#include "shaders_database.h"
#include "shaders_compiler.h"

#include "render_lines.h"      // vertex definition
#include "shaders/ps_lines.h"
#include "shaders/vs_lines.h"

#include "vertex.h"            // vertex definition
#include "shaders/vs_basic_phong_lighting.h"
#include "shaders/ps_basic_phong_lighting.h"

static const ShaderInfo::Dependency c_basic_phong_deps[] =
{
    {
        .file_name = L"" XX_SHADERS_FOLDER "common_basic_phong_lighting.hlsl"
    }
};

static const D3D11_INPUT_ELEMENT_DESC c_layout_basic_phong[] =
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
    {
        .SemanticName = "tangent",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(Vertex, tangent),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
    {
        .SemanticName = "texcoord",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(Vertex, texture_coord),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
};

extern const ShaderInfo c_vs_basic_phong
{
    .debug_name        = "vs_basic_phong",
    .kind              = ShaderInfo::VS,
    .bytecode          = {k_vs_basic_phong_lighting},
    .file_name         = L"" XX_SHADERS_FOLDER "vs_basic_phong_lighting.hlsl",
    .vs_layout         = {c_layout_basic_phong},
    .entry_point_name  = "main_vs",
    .profile          = "vs_5_0",
    .dependencies      = {c_basic_phong_deps},
    .defines           = {}
};

extern const ShaderInfo c_ps_basic_phong
{
    .debug_name         = "ps_basic_phong",
    .kind              = ShaderInfo::PS,
    .bytecode           = {k_ps_basic_phong_lighting},
    .file_name          = L"" XX_SHADERS_FOLDER "ps_basic_phong_lighting.hlsl",
    .vs_layout          = {},
    .entry_point_name   = "main_ps",
    .profile            = "ps_5_0",
    .dependencies       = {c_basic_phong_deps},
    .defines            = {}
};

static const ShaderInfo::Dependency c_lines_deps[] =
{
    {
        .file_name = L"" XX_SHADERS_FOLDER "common_ps_lines.hlsl"
    }
};

static const D3D11_INPUT_ELEMENT_DESC c_layout_lines[] =
{
    {
        .SemanticName = "position",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(RenderLines::LineVertex, position),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
    {
        .SemanticName = "color",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(RenderLines::LineVertex, color),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
};

extern const ShaderInfo c_vs_lines
{
    .debug_name        = "vs_lines",
    .kind              = ShaderInfo::VS,
    .bytecode          = {k_vs_lines},
    .file_name         = L"" XX_SHADERS_FOLDER "vs_lines.hlsl",
    .vs_layout         = {c_layout_lines},
    .entry_point_name  = "main_vs",
    .profile           = "vs_5_0",
    .dependencies      = {c_lines_deps},
    .defines           = {}
};

extern const ShaderInfo c_ps_lines
{
    .debug_name         = "ps_lines",
    .kind              = ShaderInfo::PS,
    .bytecode           = {k_ps_lines},
    .file_name          = L"" XX_SHADERS_FOLDER "ps_lines.hlsl",
    .vs_layout          = {},
    .entry_point_name   = "main_ps",
    .profile            = "ps_5_0",
    .dependencies       = {c_lines_deps},
    .defines            = {}
};
