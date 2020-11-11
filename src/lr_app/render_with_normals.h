#pragma once
#include "dx_api.h"
#include "utils.h"
#include "vertex.h"
#include "shaders_compiler.h"

#include <glm/vec3.hpp>

#include <vector>
#include <span>

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

    DirectX::XMMATRIX world;

    static RenderWithNormals make(const ComPtr<ID3D11Device>& device
        , const std::span<const NormalsVertex>& vertices);

    void render(ID3D11DeviceContext& device_context
        , const DirectX::XMMATRIX& view
        , const DirectX::XMMATRIX& projection) const;
};
