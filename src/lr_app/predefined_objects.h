#pragma once
#pragma warning(push)
// macro redefinition
#pragma warning(disable : 4005)
#include <d3d11.h>
#pragma warning(pop)
#include "utils.h"

struct RenderVertices;
struct RenderWithNormals;

RenderVertices make_cube_vertices_only(const ComPtr<ID3D11Device>& device);
RenderWithNormals make_cube_with_normals(const ComPtr<ID3D11Device>& device);
