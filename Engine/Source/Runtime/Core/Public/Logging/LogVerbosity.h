// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include <cstdint>

// -------------------------------------------------------------
// ELogVerbosity -- Log verbosity levels (REQ-016)
//
// Six severity levels from most verbose to most critical.
// Lower numeric value = higher severity.
// Fatal always prints and triggers std::abort().
// -------------------------------------------------------------

namespace Enigma
{

enum class ELogVerbosity : uint8_t
{
    Fatal   = 0,   // Unrecoverable error -- triggers abort()
    Error   = 1,   // Recoverable error
    Warning = 2,   // Potential issue
    Info    = 3,   // Informational message
    Debug   = 4,   // Debug-only message
    Verbose = 5,   // Extremely detailed trace

    // Sentinel
    NumLevels = 6,
    All       = Verbose,
};

// -- Utility functions --------------------------------------

/// Returns a short uppercase tag for the given verbosity level.
/// Example: ELogVerbosity::Warning -> "Warning"
constexpr const char* LogVerbosityToString(ELogVerbosity v) noexcept
{
    switch (v)
    {
    case ELogVerbosity::Fatal:   return "Fatal";
    case ELogVerbosity::Error:   return "Error";
    case ELogVerbosity::Warning: return "Warning";
    case ELogVerbosity::Info:    return "Info";
    case ELogVerbosity::Debug:   return "Debug";
    case ELogVerbosity::Verbose: return "Verbose";
    default:                     return "Unknown";
    }
}

/// Returns true if `messageLevel` should be displayed given `threshold`.
/// A message is shown when its severity <= threshold (lower = more severe).
constexpr bool ShouldLog(ELogVerbosity messageLevel, ELogVerbosity threshold) noexcept
{
    return static_cast<uint8_t>(messageLevel) <= static_cast<uint8_t>(threshold);
}

} // namespace Enigma
