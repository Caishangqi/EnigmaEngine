// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include <cstdint>

namespace Enigma
{

/// Primary automation test classification.
enum class EAutomationTestType : uint8_t
{
    Unit,
    Integration,
    Smoke,
    Perf,
    Functional
};

/// Additional automation test execution traits.
enum class EAutomationTestFlags : uint32_t
{
    None = 0,
    Slow = 1 << 0,
    RequiresEngine = 1 << 1,
    RequiresApplicationCore = 1 << 2,
    RequiresWindow = 1 << 3,
    RequiresProject = 1 << 4,
    Disabled = 1 << 5
};

constexpr EAutomationTestFlags operator|(
    EAutomationTestFlags Left,
    EAutomationTestFlags Right) noexcept
{
    return static_cast<EAutomationTestFlags>(
        static_cast<uint32_t>(Left) | static_cast<uint32_t>(Right));
}

constexpr EAutomationTestFlags operator&(
    EAutomationTestFlags Left,
    EAutomationTestFlags Right) noexcept
{
    return static_cast<EAutomationTestFlags>(
        static_cast<uint32_t>(Left) & static_cast<uint32_t>(Right));
}

constexpr EAutomationTestFlags& operator|=(
    EAutomationTestFlags& Left,
    EAutomationTestFlags Right) noexcept
{
    Left = Left | Right;
    return Left;
}

constexpr bool EnumHasAnyFlags(
    EAutomationTestFlags Flags,
    EAutomationTestFlags Contains) noexcept
{
    return static_cast<uint32_t>(Flags & Contains) != 0;
}

} // namespace Enigma
