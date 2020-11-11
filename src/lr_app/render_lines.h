#pragma once
#include "dx_api.h"
#include "utils.h"
#include "shaders_compiler.h"
#include "vertex.h"

#include <glm/vec3.hpp>

#include <vector>
#include <span>

// Mostly from:
// https://github.com/GPUOpen-LibrariesAndSDKs/GPUParticles11/blob/master/amd_sdk/src/LineRender.h

struct RenderLines
{
    struct LineVertex
    {
        glm::vec3 position;
        glm::vec3 color;
    };

    std::vector<LineVertex> vertices_;

    ComPtr<ID3D11Device> device_ = nullptr;
    VSShader* vs_shader_ = nullptr;
    PSShader* ps_shader_ = nullptr;
    ComPtr<ID3D11Buffer> vertex_buffer_ = nullptr;
    ComPtr<ID3D11Buffer> constant_buffer_ = nullptr;

    DirectX::XMMATRIX world;

    static RenderLines make(const ComPtr<ID3D11Device>& device);

    void add_line(const glm::vec3& p0, const glm::vec3& p1
        , const glm::vec3& color = glm::vec3(1.f));
    void add_lines(const std::span<const glm::vec3>& points
        , const glm::vec3& color = glm::vec3(1.f));
    void add_bb(const glm::vec3& min, const glm::vec3& max
        , const glm::vec3& color = glm::vec3(1.f));

    void clear();

    void render(ID3D11DeviceContext& device_context
        , const DirectX::XMMATRIX& view
        , const DirectX::XMMATRIX& projection) const;
};
