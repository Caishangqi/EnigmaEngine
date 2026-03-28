// Copyright EnigmaEngine. All Rights Reserved.

#include "Containers/Ticker.h"

#include <algorithm>

namespace Enigma
{

// -----------------------------------------------------------------
// FTSTicker
// -----------------------------------------------------------------

FTSTicker& FTSTicker::GetCoreTicker()
{
    static FTSTicker Instance;
    return Instance;
}

FDelegateHandle FTSTicker::AddTicker(const FTickerDelegate& InDelegate, float InDelay)
{
    auto Element = std::make_shared<FElement>();
    Element->Handle = FDelegateHandle::Generate();
    Element->DelayTime = InDelay;
    Element->FireTime = CurrentTime + static_cast<double>(InDelay);
    Element->Delegate = InDelegate;

    {
        std::lock_guard<std::mutex> Lock(PendingMutex);
        PendingElements.push_back(Element);
    }

    return Element->Handle;
}

void FTSTicker::RemoveTicker(FDelegateHandle InHandle)
{
    if (!InHandle.IsValid())
    {
        return;
    }

    FTSTicker& Self = GetCoreTicker();

    // Mark in active elements
    for (auto& Elem : Self.Elements)
    {
        if (Elem->Handle == InHandle)
        {
            Elem->bRemoved.store(true, std::memory_order_relaxed);
            return;
        }
    }

    // Mark in pending elements
    {
        std::lock_guard<std::mutex> Lock(Self.PendingMutex);
        for (auto& Elem : Self.PendingElements)
        {
            if (Elem->Handle == InHandle)
            {
                Elem->bRemoved.store(true, std::memory_order_relaxed);
                return;
            }
        }
    }
}

void FTSTicker::Tick(float InDeltaTime)
{
    CurrentTime += static_cast<double>(InDeltaTime);

    // Move pending elements to active list (under lock).
    {
        std::lock_guard<std::mutex> Lock(PendingMutex);
        if (!PendingElements.empty())
        {
            Elements.insert(Elements.end(),
                std::make_move_iterator(PendingElements.begin()),
                std::make_move_iterator(PendingElements.end()));
            PendingElements.clear();
        }
    }

    // Fire due delegates. Iterate by index because callbacks may add new
    // elements (which go to PendingElements, not Elements, so no invalidation).
    size_t Count = Elements.size();
    for (size_t i = 0; i < Count; ++i)
    {
        auto& Elem = Elements[i];

        if (Elem->bRemoved.load(std::memory_order_relaxed))
        {
            continue;
        }

        if (CurrentTime >= Elem->FireTime)
        {
            bool bReschedule = Elem->Delegate.Execute(InDeltaTime);

            if (bReschedule && !Elem->bRemoved.load(std::memory_order_relaxed))
            {
                // Reschedule at the same delay from now.
                Elem->FireTime = CurrentTime + static_cast<double>(Elem->DelayTime);
            }
            else
            {
                Elem->bRemoved.store(true, std::memory_order_relaxed);
            }
        }
    }

    // Compact: remove all elements marked as removed.
    Elements.erase(
        std::remove_if(Elements.begin(), Elements.end(),
            [](const std::shared_ptr<FElement>& E)
            {
                return E->bRemoved.load(std::memory_order_relaxed);
            }),
        Elements.end());
}

void FTSTicker::Reset()
{
    {
        std::lock_guard<std::mutex> Lock(PendingMutex);
        PendingElements.clear();
    }
    Elements.clear();
    CurrentTime = 0.0;
}

// -----------------------------------------------------------------
// FTSTickerObjectBase
// -----------------------------------------------------------------

FTSTickerObjectBase::FTSTickerObjectBase(float InDelay, FTSTicker& InTicker)
{
    FTickerDelegate Delegate;
    Delegate.Bind(this, &FTSTickerObjectBase::Tick);
    TickHandle = InTicker.AddTicker(Delegate, InDelay);
}

FTSTickerObjectBase::~FTSTickerObjectBase()
{
    FTSTicker::RemoveTicker(TickHandle);
}

} // namespace Enigma
