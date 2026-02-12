// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <string>

// -------------------------------------------------------------
// Assertion Macros (REQ-017)
//
// Build configurations control which assertions are active:
//
//   | Macro    | Debug | Development | Shipping |
//   |----------|-------|-------------|----------|
//   | check    |  ON   |     ON      |   OFF    |
//   | checkf   |  ON   |     ON      |   OFF    |
//   | verify   |  ON   |     ON      | expr only|
//   | ensure   |  ON   |     ON      | expr only|
//   | ensuref  |  ON   |     ON      | expr only|
//
// Semantics:
//   check(expr)          -- Fatal if expr is false. Removed entirely in Shipping.
//   checkf(expr, fmt...) -- check with formatted message.
//   verify(expr)         -- Like check, but expression always executes.
//   ensure(expr)         -- Non-fatal: logs warning, returns expr result.
//   ensuref(expr, fmt..) -- ensure with formatted message.
// -------------------------------------------------------------

// -- Build configuration guards ------------------------------
//
// ENIGMA_BUILD_SHIPPING is defined in HAL/Platform.h (defaulting to 0).
// BuildTool sets it to 1 via CMake for Shipping builds.
// DO_CHECK and DO_ENSURE are derived automatically.

#if !defined(DO_CHECK)
    #if ENIGMA_BUILD_SHIPPING
        #define DO_CHECK 0
    #else
        #define DO_CHECK 1
    #endif
#endif

#if !defined(DO_ENSURE)
    #if ENIGMA_BUILD_SHIPPING
        #define DO_ENSURE 0
    #else
        #define DO_ENSURE 1
    #endif
#endif

// -- Platform break ------------------------------------------

#if !defined(ENIGMA_DEBUG_BREAK)
    #if defined(_MSC_VER)
        #define ENIGMA_DEBUG_BREAK() __debugbreak()
    #elif defined(__clang__) || defined(__GNUC__)
        #define ENIGMA_DEBUG_BREAK() __builtin_trap()
    #else
        #define ENIGMA_DEBUG_BREAK() std::abort()
    #endif
#endif

// -- Failure handler declarations ----------------------------

namespace Enigma::Assert::Private
{

/// Called on check/verify failure. Prints message and aborts.
[[noreturn]] CORE_API void CheckFailure(
    const char* Expression,
    const char* File,
    int32_t Line,
    const std::string& Message) noexcept;

/// Called on ensure failure. Prints warning but does NOT abort.
/// Returns false (so ensure() macro can return the expression result).
CORE_API bool EnsureFailure(
    const char* Expression,
    const char* File,
    int32_t Line,
    const std::string& Message) noexcept;

} // namespace Enigma::Assert::Private

// -------------------------------------------------------------
// check / checkf -- completely removed in Shipping
// -------------------------------------------------------------

#if DO_CHECK

#define check(expr)                                                            \
    do                                                                         \
    {                                                                          \
        if (!(expr)) [[unlikely]]                                              \
        {                                                                      \
            ::Enigma::Assert::Private::CheckFailure(                           \
                #expr, __FILE__, __LINE__, "");                                \
        }                                                                      \
    } while (0)

#define checkf(expr, fmt, ...)                                                 \
    do                                                                         \
    {                                                                          \
        if (!(expr)) [[unlikely]]                                              \
        {                                                                      \
            ::Enigma::Assert::Private::CheckFailure(                           \
                #expr, __FILE__, __LINE__,                                     \
                std::format(fmt __VA_OPT__(, ) __VA_ARGS__));                  \
        }                                                                      \
    } while (0)

#else // DO_CHECK == 0  ->  check completely removed (no side effects)

#define check(expr)           do { } while (0)
#define checkf(expr, fmt, ...) do { } while (0)

#endif // DO_CHECK

// -------------------------------------------------------------
// verify -- expression ALWAYS executes, failure handling only in non-Shipping
// -------------------------------------------------------------

#if DO_CHECK

#define verify(expr)                                                           \
    do                                                                         \
    {                                                                          \
        if (!(expr)) [[unlikely]]                                              \
        {                                                                      \
            ::Enigma::Assert::Private::CheckFailure(                           \
                #expr, __FILE__, __LINE__, "");                                \
        }                                                                      \
    } while (0)

#else // DO_CHECK == 0  ->  expression executes, failure silently ignored

#define verify(expr)                                                           \
    do                                                                         \
    {                                                                          \
        (void)(expr);                                                          \
    } while (0)

#endif // DO_CHECK

// -------------------------------------------------------------
// ensure / ensuref -- non-fatal, expression ALWAYS executes,
// returns bool (the expression result).
// In Shipping: no failure reporting, just evaluates expression.
// -------------------------------------------------------------

#if DO_ENSURE

#define ensure(expr)                                                           \
    ([&]() -> bool {                                                           \
        const bool _enigma_ensure_result = !!(expr);                           \
        if (!_enigma_ensure_result) [[unlikely]]                               \
        {                                                                      \
            ::Enigma::Assert::Private::EnsureFailure(                          \
                #expr, __FILE__, __LINE__, "");                                \
        }                                                                      \
        return _enigma_ensure_result;                                          \
    }())

#define ensuref(expr, fmt, ...)                                                \
    ([&]() -> bool {                                                           \
        const bool _enigma_ensure_result = !!(expr);                           \
        if (!_enigma_ensure_result) [[unlikely]]                               \
        {                                                                      \
            ::Enigma::Assert::Private::EnsureFailure(                          \
                #expr, __FILE__, __LINE__,                                     \
                std::format(fmt __VA_OPT__(, ) __VA_ARGS__));                  \
        }                                                                      \
        return _enigma_ensure_result;                                          \
    }())

#else // DO_ENSURE == 0  ->  expression executes, no failure reporting

#define ensure(expr)            (!!(expr))
#define ensuref(expr, fmt, ...) (!!(expr))

#endif // DO_ENSURE
