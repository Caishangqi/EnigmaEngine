// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "Logging/LogCategory.h"
#include "Logging/LogVerbosity.h"

#include <cstdio>
#include <cstdlib>
#include <format>
#include <string>
#include <utility>

// -------------------------------------------------------------
// ENIGMA_LOG macro (REQ-016)
//
// Usage:
//   ENIGMA_LOG(LogCore, Info, "Loaded {} modules in {:.2f}s", count, elapsed);
//
// Design:
//   1. Global compile-time gate: ENIGMA_COMPILED_IN_MAX_VERBOSITY strips
//      Verbose/Debug in Shipping builds (zero overhead).
//   2. Per-category compile-time gate: if constexpr eliminates code when
//      the message level exceeds the category's CompileTimeVerbosity.
//   3. Runtime gate: IsSuppressed() checks current runtime level.
//   4. std::format for type-safe formatting (C++26).
//   5. Fatal level prints, then calls std::abort().
//   6. NO_LOGGING strips everything except Fatal.
// -------------------------------------------------------------

// -- Global compile-time verbosity ceiling -------------------
//
// In Shipping builds, Verbose and Debug logs are compiled out entirely.
// This is a global gate applied before per-category checks.
//
//   Shipping:    max = Info    (strips Verbose, Debug)
//   Non-Shipping: max = All   (keeps everything)

#if !defined(ENIGMA_COMPILED_IN_MAX_VERBOSITY)
    #if ENIGMA_BUILD_SHIPPING
        #define ENIGMA_COMPILED_IN_MAX_VERBOSITY ::Enigma::ELogVerbosity::Info
    #else
        #define ENIGMA_COMPILED_IN_MAX_VERBOSITY ::Enigma::ELogVerbosity::All
    #endif
#endif

namespace Enigma::Logging::Private
{

/// Core output function -- writes formatted message to stdout/stderr.
/// Exported from Core so all modules share the same output path.
CORE_API void LogOutput(
    const char* CategoryName,
    ELogVerbosity Level,
    const std::string& Message) noexcept;

/// Fatal handler -- prints message then aborts.
[[noreturn]] CORE_API void LogFatal(
    const char* CategoryName,
    const std::string& Message) noexcept;

} // namespace Enigma::Logging::Private

// -------------------------------------------------------------
// Main logging macro
// -------------------------------------------------------------

#if !defined(NO_LOGGING)

#define ENIGMA_LOG(Category, Verbosity, Fmt, ...)                                          \
    do                                                                                     \
    {                                                                                      \
        /* Global compile-time gate: strip levels above max verbosity (Shipping) */        \
        if constexpr (static_cast<uint8_t>(::Enigma::ELogVerbosity::Verbosity)             \
                      <= static_cast<uint8_t>(ENIGMA_COMPILED_IN_MAX_VERBOSITY))           \
        {                                                                                  \
        /* Per-category compile-time gate */                                                \
        if constexpr (static_cast<uint8_t>(::Enigma::ELogVerbosity::Verbosity)             \
                      <= static_cast<uint8_t>(                                             \
                             decltype(Category)::CompileTimeVerbosity))                    \
        {                                                                                  \
            /* Fatal path -- always print and abort */                                      \
            if constexpr (::Enigma::ELogVerbosity::Verbosity                               \
                          == ::Enigma::ELogVerbosity::Fatal)                               \
            {                                                                              \
                ::Enigma::Logging::Private::LogFatal(                                      \
                    Category.GetCategoryName(),                                            \
                    std::format(Fmt __VA_OPT__(, ) __VA_ARGS__));                          \
            }                                                                              \
            else                                                                           \
            {                                                                              \
                /* Runtime gate */                                                         \
                if (!Category.IsSuppressed(::Enigma::ELogVerbosity::Verbosity))            \
                {                                                                          \
                    ::Enigma::Logging::Private::LogOutput(                                 \
                        Category.GetCategoryName(),                                        \
                        ::Enigma::ELogVerbosity::Verbosity,                                \
                        std::format(Fmt __VA_OPT__(, ) __VA_ARGS__));                      \
                }                                                                          \
            }                                                                              \
        }                                                                                  \
        }                                                                                  \
    } while (0)

#else // NO_LOGGING defined -- only Fatal survives

#define ENIGMA_LOG(Category, Verbosity, Fmt, ...)                                          \
    do                                                                                     \
    {                                                                                      \
        if constexpr (::Enigma::ELogVerbosity::Verbosity                                   \
                      == ::Enigma::ELogVerbosity::Fatal)                                   \
        {                                                                                  \
            ::Enigma::Logging::Private::LogFatal(                                          \
                #Category,                                                                 \
                std::format(Fmt __VA_OPT__(, ) __VA_ARGS__));                              \
        }                                                                                  \
    } while (0)

#endif // NO_LOGGING

// -------------------------------------------------------------
// Conditional log -- only evaluates when Condition is true
// -------------------------------------------------------------

#define ENIGMA_CLOG(Condition, Category, Verbosity, Fmt, ...) \
    do                                                        \
    {                                                         \
        if (Condition)                                        \
        {                                                     \
            ENIGMA_LOG(Category, Verbosity, Fmt __VA_OPT__(, ) __VA_ARGS__); \
        }                                                     \
    } while (0)
