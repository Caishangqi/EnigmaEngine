// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include <functional>
#include <type_traits>
#include <utility>

// -------------------------------------------------------------
// TDelegate -- Single-cast delegate template.
//
// Binds exactly one callable (lambda, static function, member
// function). Unbound Execute() returns RetVal{} for non-void
// or is a no-op for void. No exceptions, no thread safety.
//
// Usage:
//   TDelegate<int(float, float)> del;
//   del.Bind([](float a, float b) { return static_cast<int>(a + b); });
//   int result = del.Execute(1.0f, 2.0f);  // 3
// -------------------------------------------------------------

namespace Enigma
{

/// Primary template (intentionally undefined).
template <typename Signature>
class TDelegate;

/// Partial specialization for function signatures.
/// @tparam RetVal  Return type of the callable.
/// @tparam Args    Argument types of the callable.
template <typename RetVal, typename... Args>
class TDelegate<RetVal(Args...)>
{
public:
    TDelegate() = default;
    ~TDelegate() = default;

    // Move semantics
    TDelegate(TDelegate&&) noexcept = default;
    TDelegate& operator=(TDelegate&&) noexcept = default;

    // Copy semantics (allowed for convenience)
    TDelegate(const TDelegate&) = default;
    TDelegate& operator=(const TDelegate&) = default;

    /// Bind a callable (lambda, function pointer, functor).
    template <typename F>
    void Bind(F&& func)
    {
        m_callback = std::forward<F>(func);
    }

    /// Bind a member function.
    template <typename T, typename Method>
    void Bind(T* instance, Method method)
    {
        m_callback = [instance, method](Args... args) -> RetVal
        {
            return (instance->*method)(std::forward<Args>(args)...);
        };
    }

    /// Clear the current binding.
    void Unbind()
    {
        m_callback = nullptr;
    }

    /// Check if a callable is bound.
    [[nodiscard]] bool IsBound() const
    {
        return static_cast<bool>(m_callback);
    }

    /// Explicit bool conversion.
    explicit operator bool() const
    {
        return IsBound();
    }

    /// Execute the delegate.
    /// Returns RetVal{} if unbound (no-op for void).
    RetVal Execute(Args... args) const
    {
        if (m_callback)
        {
            return m_callback(std::forward<Args>(args)...);
        }

        if constexpr (!std::is_void_v<RetVal>)
        {
            return RetVal{};
        }
    }

    /// Execute only if bound. Returns true if executed.
    bool ExecuteIfBound(Args... args) const
    {
        if (m_callback)
        {
            m_callback(std::forward<Args>(args)...);
            return true;
        }
        return false;
    }

private:
    std::function<RetVal(Args...)> m_callback;
};

} // namespace Enigma
