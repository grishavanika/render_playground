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
