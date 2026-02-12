// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"
#include "Logging/LogVerbosity.h"

#include <cstdint>
#include <string_view>

// -------------------------------------------------------------
// Log Category System (REQ-016)
//
// Each log category has:
//   - A compile-time verbosity (template param, enables zero-overhead removal)
//   - A runtime verbosity (adjustable, never exceeds compile-time)
//   - A name string for output prefix
//
// Usage pattern:
//   DECLARE_LOG_CATEGORY(LogCore, Info, All)   // in header
//   DEFINE_LOG_CATEGORY(LogCore)               // in one .cpp
// -------------------------------------------------------------

namespace Enigma
{

/// Base class for all log categories. Holds runtime state.
class CORE_API FLogCategoryBase
{
public:
    FLogCategoryBase(
        const char* InName,
        ELogVerbosity InDefaultVerbosity,
        ELogVerbosity InCompileTimeVerbosity) noexcept;

    ~FLogCategoryBase() noexcept = default;

    /// Check if a message at the given level should be suppressed.
    [[nodiscard]] constexpr bool IsSuppressed(ELogVerbosity Level) const noexcept
    {
        return !ShouldLog(Level, RuntimeVerbosity);
    }

    [[nodiscard]] constexpr const char* GetCategoryName() const noexcept
    {
        return Name;
    }

    [[nodiscard]] constexpr ELogVerbosity GetVerbosity() const noexcept
    {
        return RuntimeVerbosity;
    }

    [[nodiscard]] constexpr ELogVerbosity GetCompileTimeVerbosity() const noexcept
    {
        return CompileTimeVerbosity;
    }

    /// Set runtime verbosity. Clamped to [Fatal, CompileTimeVerbosity].
    void SetVerbosity(ELogVerbosity NewVerbosity) noexcept;

private:
    const char*   Name;
    ELogVerbosity RuntimeVerbosity;
    ELogVerbosity DefaultVerbosity;
    ELogVerbosity CompileTimeVerbosity;
};

// -------------------------------------------------------------

/// Typed log category with compile-time verbosity for zero-overhead filtering.
template <ELogVerbosity InDefaultVerbosity, ELogVerbosity InCompileTimeVerbosity>
class FLogCategory : public FLogCategoryBase
{
    static_assert(static_cast<uint8_t>(InDefaultVerbosity) < static_cast<uint8_t>(ELogVerbosity::NumLevels),
                  "Default verbosity out of range.");
    static_assert(static_cast<uint8_t>(InCompileTimeVerbosity) < static_cast<uint8_t>(ELogVerbosity::NumLevels),
                  "Compile-time verbosity out of range.");
    static_assert(static_cast<uint8_t>(InDefaultVerbosity) <= static_cast<uint8_t>(InCompileTimeVerbosity),
                  "Default verbosity must not exceed compile-time verbosity.");

public:
    static constexpr ELogVerbosity CompileTimeVerbosity = InCompileTimeVerbosity;

    explicit FLogCategory(const char* InName) noexcept
        : FLogCategoryBase(InName, InDefaultVerbosity, InCompileTimeVerbosity)
    {
    }
};

} // namespace Enigma

// -------------------------------------------------------------
// Convenience macros for declaring / defining log categories
// -------------------------------------------------------------

/// Declare a log category (typically in a header).
/// Example: DECLARE_LOG_CATEGORY(LogCore, Info, All)
#define DECLARE_LOG_CATEGORY(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
    extern struct FLogCategory_##CategoryName                                      \
        : public ::Enigma::FLogCategory<                                           \
              ::Enigma::ELogVerbosity::DefaultVerbosity,                           \
              ::Enigma::ELogVerbosity::CompileTimeVerbosity>                       \
    {                                                                              \
        FLogCategory_##CategoryName()                                              \
            : ::Enigma::FLogCategory<                                              \
                  ::Enigma::ELogVerbosity::DefaultVerbosity,                       \
                  ::Enigma::ELogVerbosity::CompileTimeVerbosity>(#CategoryName)    \
        {                                                                          \
        }                                                                          \
    } CategoryName

/// Define a log category (in exactly one .cpp).
#define DEFINE_LOG_CATEGORY(CategoryName) \
    FLogCategory_##CategoryName CategoryName

/// Declare + define a file-local (static) log category.
#define DEFINE_LOG_CATEGORY_STATIC(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
    static struct FLogCategory_##CategoryName                                            \
        : public ::Enigma::FLogCategory<                                                 \
              ::Enigma::ELogVerbosity::DefaultVerbosity,                                 \
              ::Enigma::ELogVerbosity::CompileTimeVerbosity>                             \
    {                                                                                    \
        FLogCategory_##CategoryName()                                                    \
            : ::Enigma::FLogCategory<                                                    \
                  ::Enigma::ELogVerbosity::DefaultVerbosity,                             \
                  ::Enigma::ELogVerbosity::CompileTimeVerbosity>(#CategoryName)          \
        {                                                                                \
        }                                                                                \
    } CategoryName
