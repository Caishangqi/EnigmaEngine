// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "ApplicationCoreAPI.generated.h"

#include <cstdint>

// -------------------------------------------------------------
// FGenericWindow
//
// Platform-agnostic window abstraction. Concrete implementations
// (e.g. FWindowsWindow) override all pure virtual methods.
// -------------------------------------------------------------

namespace Enigma
{

/// Abstract window interface.
class APPLICATIONCORE_API FGenericWindow
{
public:
    virtual ~FGenericWindow() = default;

    /// Show the window.
    virtual void Show() = 0;

    /// Hide the window.
    virtual void Hide() = 0;

    /// Destroy the window and release OS resources.
    virtual void Destroy() = 0;

    /// Minimize the window.
    virtual void Minimize() = 0;

    /// Maximize the window.
    virtual void Maximize() = 0;

    /// Restore the window from minimized/maximized state.
    virtual void Restore() = 0;

    /// Set the window title bar text.
    virtual void SetTitle(const char* title) = 0;

    /// Resize the window client area.
    virtual void Resize(int32_t width, int32_t height) = 0;

    /// Move the window to the specified screen position.
    virtual void Move(int32_t x, int32_t y) = 0;

    /// Get the platform-specific native handle (e.g. HWND on Windows).
    virtual void* GetNativeHandle() const = 0;

    /// Check if the window is currently visible.
    virtual bool IsVisible() const = 0;

    /// Check if the window currently has input focus.
    virtual bool HasFocus() const = 0;

    /// Get the window client area width in pixels.
    virtual int32_t GetWidth() const = 0;

    /// Get the window client area height in pixels.
    virtual int32_t GetHeight() const = 0;
};

} // namespace Enigma
