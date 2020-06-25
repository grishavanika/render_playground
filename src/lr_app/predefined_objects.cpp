#include "predefined_objects.h"
#include "render_vertices_only.h"
#include "render_with_normals.h"

static const Vector3f c_cube_vertices[] =
{
    Vector3f{-0.5f, -0.5f, -0.5f},
    Vector3f{ 0.5f, -0.5f, -0.5f},
    Vector3f{ 0.5f,  0.5f, -0.5f},
    Vector3f{ 0.5f,  0.5f, -0.5f},
    Vector3f{-0.5f,  0.5f, -0.5f},
    Vector3f{-0.5f, -0.5f, -0.5f},
    Vector3f{-0.5f, -0.5f,  0.5f},
    Vector3f{ 0.5f, -0.5f,  0.5f},
    Vector3f{ 0.5f,  0.5f,  0.5f},
    Vector3f{ 0.5f,  0.5f,  0.5f},
    Vector3f{-0.5f,  0.5f,  0.5f},
    Vector3f{-0.5f, -0.5f,  0.5f},
    Vector3f{-0.5f,  0.5f,  0.5f},
    Vector3f{-0.5f,  0.5f, -0.5f},
    Vector3f{-0.5f, -0.5f, -0.5f},
    Vector3f{-0.5f, -0.5f, -0.5f},
    Vector3f{-0.5f, -0.5f,  0.5f},
    Vector3f{-0.5f,  0.5f,  0.5f},
    Vector3f{ 0.5f,  0.5f,  0.5f},
    Vector3f{ 0.5f,  0.5f, -0.5f},
    Vector3f{ 0.5f, -0.5f, -0.5f},
    Vector3f{ 0.5f, -0.5f, -0.5f},
    Vector3f{ 0.5f, -0.5f,  0.5f},
    Vector3f{ 0.5f,  0.5f,  0.5f},
    Vector3f{-0.5f, -0.5f, -0.5f},
    Vector3f{ 0.5f, -0.5f, -0.5f},
    Vector3f{ 0.5f, -0.5f,  0.5f},
    Vector3f{ 0.5f, -0.5f,  0.5f},
    Vector3f{-0.5f, -0.5f,  0.5f},
    Vector3f{-0.5f, -0.5f, -0.5f},
    Vector3f{-0.5f,  0.5f, -0.5f},
    Vector3f{ 0.5f,  0.5f, -0.5f},
    Vector3f{ 0.5f,  0.5f,  0.5f},
    Vector3f{ 0.5f,  0.5f,  0.5f},
    Vector3f{-0.5f,  0.5f,  0.5f},
    Vector3f{-0.5f,  0.5f, -0.5f}
};

static RenderWithNormals::NormalsVertex c_cube_normals[] =
{
    {Vector3f{-0.5f, -0.5f, -0.5f},  Vector3f{0.0f,  0.0f, -1.0f}},
    {Vector3f{ 0.5f, -0.5f, -0.5f},  Vector3f{0.0f,  0.0f, -1.0f}},
    {Vector3f{ 0.5f,  0.5f, -0.5f},  Vector3f{0.0f,  0.0f, -1.0f}},
    {Vector3f{ 0.5f,  0.5f, -0.5f},  Vector3f{0.0f,  0.0f, -1.0f}},
    {Vector3f{-0.5f,  0.5f, -0.5f},  Vector3f{0.0f,  0.0f, -1.0f}},
    {Vector3f{-0.5f, -0.5f, -0.5f},  Vector3f{0.0f,  0.0f, -1.0f}},
    {Vector3f{-0.5f, -0.5f,  0.5f},  Vector3f{0.0f,  0.0f,  1.0f}},
    {Vector3f{ 0.5f, -0.5f,  0.5f},  Vector3f{0.0f,  0.0f,  1.0f}},
    {Vector3f{ 0.5f,  0.5f,  0.5f},  Vector3f{0.0f,  0.0f,  1.0f}},
    {Vector3f{ 0.5f,  0.5f,  0.5f},  Vector3f{0.0f,  0.0f,  1.0f}},
    {Vector3f{-0.5f,  0.5f,  0.5f},  Vector3f{0.0f,  0.0f,  1.0f}},
    {Vector3f{-0.5f, -0.5f,  0.5f},  Vector3f{0.0f,  0.0f,  1.0f}},
    {Vector3f{-0.5f,  0.5f,  0.5f},  Vector3f{-1.0f, 0.0f,  0.0f}},
    {Vector3f{-0.5f,  0.5f, -0.5f},  Vector3f{-1.0f, 0.0f,  0.0f}},
    {Vector3f{-0.5f, -0.5f, -0.5f},  Vector3f{-1.0f, 0.0f,  0.0f}},
    {Vector3f{-0.5f, -0.5f, -0.5f},  Vector3f{-1.0f, 0.0f,  0.0f}},
    {Vector3f{-0.5f, -0.5f,  0.5f},  Vector3f{-1.0f, 0.0f,  0.0f}},
    {Vector3f{-0.5f,  0.5f,  0.5f},  Vector3f{-1.0f, 0.0f,  0.0f}},
    {Vector3f{ 0.5f,  0.5f,  0.5f},  Vector3f{1.0f,  0.0f,  0.0f}},
    {Vector3f{ 0.5f,  0.5f, -0.5f},  Vector3f{1.0f,  0.0f,  0.0f}},
    {Vector3f{ 0.5f, -0.5f, -0.5f},  Vector3f{1.0f,  0.0f,  0.0f}},
    {Vector3f{ 0.5f, -0.5f, -0.5f},  Vector3f{1.0f,  0.0f,  0.0f}},
    {Vector3f{ 0.5f, -0.5f,  0.5f},  Vector3f{1.0f,  0.0f,  0.0f}},
    {Vector3f{ 0.5f,  0.5f,  0.5f},  Vector3f{1.0f,  0.0f,  0.0f}},
    {Vector3f{-0.5f, -0.5f, -0.5f},  Vector3f{0.0f, -1.0f,  0.0f}},
    {Vector3f{ 0.5f, -0.5f, -0.5f},  Vector3f{0.0f, -1.0f,  0.0f}},
    {Vector3f{ 0.5f, -0.5f,  0.5f},  Vector3f{0.0f, -1.0f,  0.0f}},
    {Vector3f{ 0.5f, -0.5f,  0.5f},  Vector3f{0.0f, -1.0f,  0.0f}},
    {Vector3f{-0.5f, -0.5f,  0.5f},  Vector3f{0.0f, -1.0f,  0.0f}},
    {Vector3f{-0.5f, -0.5f, -0.5f},  Vector3f{0.0f, -1.0f,  0.0f}},
    {Vector3f{-0.5f,  0.5f, -0.5f},  Vector3f{0.0f,  1.0f,  0.0f}},
    {Vector3f{ 0.5f,  0.5f, -0.5f},  Vector3f{0.0f,  1.0f,  0.0f}},
    {Vector3f{ 0.5f,  0.5f,  0.5f},  Vector3f{0.0f,  1.0f,  0.0f}},
    {Vector3f{ 0.5f,  0.5f,  0.5f},  Vector3f{0.0f,  1.0f,  0.0f}},
    {Vector3f{-0.5f,  0.5f,  0.5f},  Vector3f{0.0f,  1.0f,  0.0f}},
    {Vector3f{-0.5f,  0.5f, -0.5f},  Vector3f{0.0f,  1.0f,  0.0f}},
};

RenderVertices make_cube_vertices_only(const ComPtr<ID3D11Device>& device)
{
    return RenderVertices::make(device, c_cube_vertices);
}

RenderWithNormals make_cube_with_normals(const ComPtr<ID3D11Device>& device)
{
    return RenderWithNormals::make(device, c_cube_normals);
}
