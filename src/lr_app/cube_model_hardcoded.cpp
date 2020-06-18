#include "vertex.h"

void CubeModel_Predefined()
{
    const Vertex vertices_raw[] =
    {
        {Vector3f{-1.0f, 1.0f, -1.0f}, Vector3f{0.0f, 1.0f, 0.0f}, Vector2f{0.0f, 0.0f}},
        {Vector3f{1.0f, 1.0f, -1.0f}, Vector3f{0.0f, 1.0f, 0.0f}, Vector2f{1.0f, 0.0f}},
        {Vector3f{1.0f, 1.0f, 1.0f}, Vector3f{0.0f, 1.0f, 0.0f}, Vector2f{1.0f, 1.0f}},
        {Vector3f{-1.0f, 1.0f, 1.0f}, Vector3f{0.0f, 1.0f, 0.0f}, Vector2f{0.0f, 1.0f}},

        {Vector3f{-1.0f, -1.0f, -1.0f}, Vector3f{0.0f, -1.0f, 0.0f}, Vector2f{0.0f, 0.0f}},
        {Vector3f{1.0f, -1.0f, -1.0f}, Vector3f{0.0f, -1.0f, 0.0f}, Vector2f{1.0f, 0.0f}},
        {Vector3f{1.0f, -1.0f, 1.0f}, Vector3f{0.0f, -1.0f, 0.0f}, Vector2f{1.0f, 1.0f}},
        {Vector3f{-1.0f, -1.0f, 1.0f}, Vector3f{0.0f, -1.0f, 0.0f}, Vector2f{0.0f, 1.0f}},

        {Vector3f{-1.0f, -1.0f, 1.0f}, Vector3f{-1.0f, 0.0f, 0.0f}, Vector2f{0.0f, 0.0f}},
        {Vector3f{-1.0f, -1.0f, -1.0f}, Vector3f{-1.0f, 0.0f, 0.0f}, Vector2f{1.0f, 0.0f}},
        {Vector3f{-1.0f, 1.0f, -1.0f}, Vector3f{-1.0f, 0.0f, 0.0f}, Vector2f{1.0f, 1.0f}},
        {Vector3f{-1.0f, 1.0f, 1.0f}, Vector3f{-1.0f, 0.0f, 0.0f}, Vector2f{0.0f, 1.0f}},

        {Vector3f{1.0f, -1.0f, 1.0f}, Vector3f{1.0f, 0.0f, 0.0f}, Vector2f{0.0f, 0.0f}},
        {Vector3f{1.0f, -1.0f, -1.0f}, Vector3f{1.0f, 0.0f, 0.0f}, Vector2f{1.0f, 0.0f}},
        {Vector3f{1.0f, 1.0f, -1.0f}, Vector3f{1.0f, 0.0f, 0.0f}, Vector2f{1.0f, 1.0f}},
        {Vector3f{1.0f, 1.0f, 1.0f}, Vector3f{1.0f, 0.0f, 0.0f}, Vector2f{0.0f, 1.0f}},

        {Vector3f{-1.0f, -1.0f, -1.0f}, Vector3f{0.0f, 0.0f, -1.0f}, Vector2f{0.0f, 0.0f}},
        {Vector3f{1.0f, -1.0f, -1.0f}, Vector3f{0.0f, 0.0f, -1.0f}, Vector2f{1.0f, 0.0f}},
        {Vector3f{1.0f, 1.0f, -1.0f}, Vector3f{0.0f, 0.0f, -1.0f}, Vector2f{1.0f, 1.0f}},
        {Vector3f{-1.0f, 1.0f, -1.0f}, Vector3f{0.0f, 0.0f, -1.0f}, Vector2f{0.0f, 1.0f}},

        {Vector3f{-1.0f, -1.0f, 1.0f}, Vector3f{0.0f, 0.0f, 1.0f}, Vector2f{0.0f, 0.0f}},
        {Vector3f{1.0f, -1.0f, 1.0f}, Vector3f{0.0f, 0.0f, 1.0f}, Vector2f{1.0f, 0.0f}},
        {Vector3f{1.0f, 1.0f, 1.0f}, Vector3f{0.0f, 0.0f, 1.0f}, Vector2f{1.0f, 1.0f}},
        {Vector3f{-1.0f, 1.0f, 1.0f}, Vector3f{0.0f, 0.0f, 1.0f}, Vector2f{0.0f, 1.0f}},
    };

    const Index indices_raw[] =
    {
        3, 1, 0,
        2, 1, 3,

        6, 4, 5,
        7, 4, 6,

        11, 9, 8,
        10, 9, 11,

        14, 12, 13,
        15, 12, 14,

        19, 17, 16,
        18, 17, 19,

        22, 20, 21,
        23, 20, 22
    };

    (void)vertices_raw;
    (void)indices_raw;
}
