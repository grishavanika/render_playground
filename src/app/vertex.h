#pragma once
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

static_assert(sizeof(float) == 4);

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal; // optional
    glm::vec3 tangent; // optional
    glm::vec2 texture_coord; // diffuse, optional
};

using Index = unsigned int;
static_assert(sizeof(Index) == 4); // To avoid including <cstdint>
