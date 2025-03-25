#pragma once
#include <algorithm>

#include <cassert>
#include <cstdlib>

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
