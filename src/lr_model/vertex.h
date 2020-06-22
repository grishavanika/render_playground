#pragma once

static_assert(sizeof(float) == 4);

struct Vector3f
{
    float x;
    float y;
    float z;
};

struct Vector2f
{
    float x;
    float y;
};

struct Vertex
{
    Vector3f position;
    Vector3f normal;
    Vector3f tangent;
    Vector2f texture_coord; // diffuse
};

using Index = unsigned int;
static_assert(sizeof(Index) == 4); // To avoid including <cstdint>
