// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"
#include "Delegates/Delegate.h"
#include "Delegates/DelegateHandle.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

// -------------------------------------------------------------
// FTSTicker -- Thread-safe frame-level ticker system.
//
// Any subsystem can register per-frame callbacks via AddTicker().
// Driven by FEngineLoop::Tick() calling Tick(dt) once per frame.
// Matches UE's FTSTicker interface.
//
// Usage:
//   auto handle = FTSTicker::GetCoreTicker().AddTicker(
//       FTickerDelegate::CreateLambda([](float dt) { return true; }));
//   // ... later ...
//   FTSTicker::RemoveTicker(handle);
// -------------------------------------------------------------

namespace Enigma
{

/// Ticker delegate: return true to reschedule, false for one-shot.
using FTickerDelegate = TDelegate<bool(float)>;

/// Thread-safe frame-level ticker. Fires delegates after a delay.
class CORE_API FTSTicker
{
public:
    /// Singleton accessor.
    static FTSTicker& GetCoreTicker();

    /// Register a callback. delay=0 means "next frame".
    /// Thread-safe: can be called from any thread.
    FDelegateHandle AddTicker(const FTickerDelegate& InDelegate, float InDelay = 0.0f);

    /// Unregister a callback. Thread-safe.
    static void RemoveTicker(FDelegateHandle InHandle);

    /// Fire all due delegates. Must be called from main thread only.
    void Tick(float InDeltaTime);

    /// Reset all state (removes all elements).
    void Reset();

private:
    struct FElement
    {
        FDelegateHandle         Handle;
        double                  FireTime;
        float                   DelayTime;
        FTickerDelegate         Delegate;
        std::atomic<bool>       bRemoved{false};

        FElement() = default;
        ~FElement() = default;
        FElement(const FElement&) = delete;
        FElement& operator=(const FElement&) = delete;
    };

    std::mutex                                  PendingMutex;
    std::vector<std::shared_ptr<FElement>>      PendingElements;
    std::vector<std::shared_ptr<FElement>>      Elements;
    double                                      CurrentTime = 0.0;
};

/// Convenience base class: auto-registers with ticker on construction,
/// auto-unregisters on destruction. Subclass and override Tick().
class CORE_API FTSTickerObjectBase
{
public:
    FTSTickerObjectBase(float InDelay = 0.0f, FTSTicker& InTicker = FTSTicker::GetCoreTicker());
    virtual ~FTSTickerObjectBase();

    FTSTickerObjectBase(const FTSTickerObjectBase&) = delete;
    FTSTickerObjectBase& operator=(const FTSTickerObjectBase&) = delete;

    /// Override this. Return true to keep ticking, false to stop.
    virtual bool Tick(float InDeltaTime) = 0;

private:
    FDelegateHandle TickHandle;
};

} // namespace Enigma
