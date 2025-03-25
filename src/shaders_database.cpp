#include "shaders_database.h"
#include "shaders_compiler.h"

#include "render_lines.h"      // vertex definition
#include "shaders/ps_lines.h"
#include "shaders/vs_lines.h"

#include "vertex.h"            // vertex definition
#include "shaders/vs_basic_phong_lighting.h"
#include "shaders/ps_basic_phong_lighting.h"

#include "shaders/vs_gooch_shading.h"
#include "shaders/ps_gooch_shading.h"

#include "shaders/vs_vertices_only.h"
#include "shaders/ps_vertices_only.h"

#include "render_with_normals.h"  // vertex definition
#include "shaders/vs_normals.h"
#include "shaders/ps_normals.h"

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
    .profile           = "vs_5_0",
    .dependencies      = {c_basic_phong_deps},
    .defines           = {}
};

static const ShaderInfo::Dependency c_gooch_shading_deps[] =
{
    {
        .file_name = L"" XX_SHADERS_FOLDER "common_gooch_shading.hlsl"
    }
};

static const D3D11_INPUT_ELEMENT_DESC c_layout_gooch_shading[] =
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

extern const ShaderInfo c_vs_gooch_shading
{
    .debug_name        = "vs_gooch_shading",
    .kind              = ShaderInfo::VS,
    .bytecode          = {k_vs_gooch_shading},
    .file_name         = L"" XX_SHADERS_FOLDER "vs_gooch_shading.hlsl",
    .vs_layout         = {c_layout_gooch_shading},
    .entry_point_name  = "main_vs",
    .profile           = "vs_5_0",
    .dependencies      = {c_gooch_shading_deps},
    .defines           = {}
};

extern const ShaderInfo c_ps_basic_phong
{
    .debug_name         = "ps_basic_phong",
    .kind               = ShaderInfo::PS,
    .bytecode           = {k_ps_basic_phong_lighting},
    .file_name          = L"" XX_SHADERS_FOLDER "ps_basic_phong_lighting.hlsl",
    .vs_layout          = {},
    .entry_point_name   = "main_ps",
    .profile            = "ps_5_0",
    .dependencies       = {c_basic_phong_deps},
    .defines            = {}
};

extern const ShaderInfo c_ps_gooch_shading
{
    .debug_name         = "ps_gooch_shading",
    .kind               = ShaderInfo::PS,
    .bytecode           = {k_ps_gooch_shading},
    .file_name          = L"" XX_SHADERS_FOLDER "ps_gooch_shading.hlsl",
    .vs_layout          = {},
    .entry_point_name   = "main_ps",
    .profile            = "ps_5_0",
    .dependencies       = {c_gooch_shading_deps},
    .defines            = {}
};

static const ShaderInfo::Dependency c_lines_deps[] =
{
    {
        .file_name = L"" XX_SHADERS_FOLDER "common_lines.hlsl"
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

static const D3D11_INPUT_ELEMENT_DESC c_layout_vertices_only[] =
{
    {
        .SemanticName = "position",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = 0,
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
};

extern const ShaderInfo c_vs_vertices_only
{
    .debug_name        = "vs_vertices_only",
    .kind              = ShaderInfo::VS,
    .bytecode          = {k_vs_vertices_only},
    .file_name         = L"" XX_SHADERS_FOLDER "vs_vertices_only.hlsl",
    .vs_layout         = {c_layout_vertices_only},
    .entry_point_name  = "main_vs",
    .profile           = "vs_5_0",
    .dependencies      = {},
    .defines           = {}
};

extern const ShaderInfo c_ps_vertices_only
{
    .debug_name         = "ps_vertices_only",
    .kind              = ShaderInfo::PS,
    .bytecode           = {k_ps_vertices_only},
    .file_name          = L"" XX_SHADERS_FOLDER "ps_vertices_only.hlsl",
    .vs_layout          = {},
    .entry_point_name   = "main_ps",
    .profile            = "ps_5_0",
    .dependencies       = {},
    .defines            = {}
};

static const D3D11_INPUT_ELEMENT_DESC c_layout_normals[] =
{
    {
        .SemanticName = "position",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(RenderWithNormals::NormalsVertex, position),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
    {
        .SemanticName = "normal",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = offsetof(RenderWithNormals::NormalsVertex, normal),
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    },
};

extern const ShaderInfo c_vs_normals
{
    .debug_name        = "vs_normals",
    .kind              = ShaderInfo::VS,
    .bytecode          = {k_vs_normals},
    .file_name         = L"" XX_SHADERS_FOLDER "vs_normals.hlsl",
    .vs_layout         = {c_layout_normals},
    .entry_point_name  = "main_vs",
    .profile           = "vs_5_0",
    .dependencies      = {},
    .defines           = {}
};

extern const ShaderInfo c_ps_normals
{
    .debug_name         = "ps_normals",
    .kind               = ShaderInfo::PS,
    .bytecode           = {k_ps_normals},
    .file_name          = L"" XX_SHADERS_FOLDER "ps_normals.hlsl",
    .vs_layout          = {},
    .entry_point_name   = "main_ps",
    .profile            = "ps_5_0",
    .dependencies       = {},
    .defines            = {}
};
