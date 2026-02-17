// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"

#include <cstdint>

// -------------------------------------------------------------
// FDelegateHandle
//
// Type-safe handle for identifying delegate bindings.
// Value 0 is reserved as "invalid/empty".
// Used by TMulticastDelegate for listener removal.
// -------------------------------------------------------------

namespace Enigma
{

/// Type-safe handle for delegate listener identification.
/// Wraps a uint64_t; value 0 means invalid/empty.
class CORE_API FDelegateHandle
{
public:
    FDelegateHandle() = default;
    ~FDelegateHandle() = default;

    FDelegateHandle(const FDelegateHandle&) = default;
    FDelegateHandle& operator=(const FDelegateHandle&) = default;
    FDelegateHandle(FDelegateHandle&&) noexcept = default;
    FDelegateHandle& operator=(FDelegateHandle&&) noexcept = default;

    /// Check if this handle refers to a valid binding.
    [[nodiscard]] bool IsValid() const noexcept;

    /// Reset this handle to the invalid state.
    void Reset() noexcept;

    bool operator==(const FDelegateHandle&) const = default;
    bool operator!=(const FDelegateHandle&) const = default;

    /// Generate a new unique handle (single-threaded, no atomics).
    static FDelegateHandle Generate();

private:
    explicit FDelegateHandle(uint64_t InId) noexcept;

    uint64_t m_id = 0;
    static uint64_t s_nextId;
};

} // namespace Enigma
