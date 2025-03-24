#pragma once
#undef min
#undef max
#include <outcome.hpp>
namespace outcome = OUTCOME_V2_NAMESPACE;

#include <system_error>

namespace todo
{
    constexpr std::errc not_implemented = std::errc::not_supported;
} // namespace todo
