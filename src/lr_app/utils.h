#pragma once
#include <algorithm>

#include <cstdlib>
#include <cassert>

inline void Panic(bool condition)
{
    if (!condition)
    {
        assert(false);
        std::exit(1);
    }
}

[[noreturn]] inline void Unreachable()
{
    assert(false);
    std::exit(1);
}

inline float DegreesToRadians(float degrees)
{
    const float degree_to_radian = 0.01745329252f; // 3.14/180
    return (degrees * degree_to_radian);
}
