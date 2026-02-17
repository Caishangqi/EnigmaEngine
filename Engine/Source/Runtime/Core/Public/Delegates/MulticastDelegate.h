// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "Delegates/DelegateHandle.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

// -------------------------------------------------------------
// TMulticastDelegate -- Multi-cast delegate template.
//
// Broadcasts to multiple listeners. Return type is always void.
// Listeners are identified by FDelegateHandle for safe removal.
// Broadcast uses copy-on-iterate for safety when listeners
// modify the list during broadcast.
//
// Usage:
//   TMulticastDelegate<int, float> onEvent;
//   auto h = onEvent.Add([](int a, float b) { /* ... */ });
//   onEvent.Broadcast(42, 3.14f);
//   onEvent.Remove(h);
// -------------------------------------------------------------

namespace Enigma
{

/// Multi-cast delegate -- broadcasts to multiple listeners.
/// Return type is always void.
/// @tparam Args  Callback argument types.
template <typename... Args>
class TMulticastDelegate
{
public:
    TMulticastDelegate() = default;
    ~TMulticastDelegate() = default;

    // Move-only (listeners contain state)
    TMulticastDelegate(TMulticastDelegate&&) noexcept = default;
    TMulticastDelegate& operator=(TMulticastDelegate&&) noexcept = default;
    TMulticastDelegate(const TMulticastDelegate&) = delete;
    TMulticastDelegate& operator=(const TMulticastDelegate&) = delete;

    /// Add a callable listener. Returns handle for removal.
    template <typename F>
    FDelegateHandle Add(F&& func)
    {
        FDelegateHandle handle = FDelegateHandle::Generate();
        m_listeners.push_back(FListener{
            handle,
            std::forward<F>(func),
            nullptr
        });
        return handle;
    }

    /// Add a member function listener. Returns handle for removal.
    template <typename T, typename Method>
    FDelegateHandle Add(T* instance, Method method)
    {
        FDelegateHandle handle = FDelegateHandle::Generate();
        m_listeners.push_back(FListener{
            handle,
            [instance, method](Args... args)
            {
                (instance->*method)(std::forward<Args>(args)...);
            },
            static_cast<const void*>(instance)
        });
        return handle;
    }

    /// Remove a listener by handle.
    /// @return true if a listener was found and removed.
    bool Remove(FDelegateHandle handle)
    {
        auto it = std::find_if(
            m_listeners.begin(),
            m_listeners.end(),
            [handle](const FListener& listener)
            {
                return listener.Handle == handle;
            });

        if (it != m_listeners.end())
        {
            m_listeners.erase(it);
            return true;
        }
        return false;
    }

    /// Remove all listeners bound via a specific object pointer.
    void RemoveAll(const void* object)
    {
        std::erase_if(m_listeners,
            [object](const FListener& listener)
            {
                return listener.BoundObject == object;
            });
    }

    /// Remove all listeners.
    void Clear()
    {
        m_listeners.clear();
    }

    /// Get listener count.
    [[nodiscard]] int32_t GetCount() const
    {
        return static_cast<int32_t>(m_listeners.size());
    }

    /// Check if any listeners are bound.
    [[nodiscard]] bool IsBound() const
    {
        return !m_listeners.empty();
    }

    /// Broadcast to all listeners (copy-safe iteration).
    /// A local copy of the listener list is made before iterating,
    /// so listeners may safely Add/Remove during broadcast.
    void Broadcast(Args... args) const
    {
        // Copy-on-iterate: safe if listeners modify the list
        auto copy = m_listeners;
        for (const auto& listener : copy)
        {
            listener.Callback(std::forward<Args>(args)...);
        }
    }

private:
    struct FListener
    {
        FDelegateHandle Handle;
        std::function<void(Args...)> Callback;
        const void* BoundObject = nullptr;
    };

    std::vector<FListener> m_listeners;
};

} // namespace Enigma
