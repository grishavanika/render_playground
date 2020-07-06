#pragma once
#include "dx_api.h"

struct RenderVertices;
struct RenderWithNormals;

RenderVertices make_cube_vertices_only(const ComPtr<ID3D11Device>& device);
RenderWithNormals make_cube_with_normals(const ComPtr<ID3D11Device>& device);
