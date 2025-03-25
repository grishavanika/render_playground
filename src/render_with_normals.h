#pragma once
#include "dx_api.h"
#include "shaders_compiler.h"
#include "utils.h"
#include "vertex.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <span>
#include <vector>

struct RenderWithNormals
{
    struct NormalsVertex
    {
        glm::vec3 position;
        glm::vec3 normal;
    };

    std::vector<NormalsVertex> vertices_;

    ComPtr<ID3D11Device> device_ = nullptr;
    VSShader* vs_shader_ = nullptr;
    PSShader* ps_shader_ = nullptr;
    ComPtr<ID3D11Buffer> vertex_buffer_ = nullptr;
    ComPtr<ID3D11Buffer> constant_buffer_ = nullptr;

    glm::mat4x4 world;

    static RenderWithNormals make(const ComPtr<ID3D11Device>& device, const std::span<const NormalsVertex>& vertices);

    void render(ID3D11DeviceContext& device_context, const glm::mat4x4& view, const glm::mat4x4& projection) const;
};
