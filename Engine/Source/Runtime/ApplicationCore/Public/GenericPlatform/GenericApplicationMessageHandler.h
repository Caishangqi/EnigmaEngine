// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "ApplicationCoreAPI.generated.h"

#include <cstdint>

// -------------------------------------------------------------
// FGenericApplicationMessageHandler
//
// Abstract interface defining all input and window event
// callbacks. Platform implementations translate OS messages
// to these virtual calls. Higher-level systems (future
// Slate/Input modules) override to receive events.
//
// All input methods return bool: true = consumed, false = pass.
// Window events return void (always processed).
// Default implementations are no-ops.
// -------------------------------------------------------------

namespace Enigma
{

class FGenericWindow;

/// Mouse button identifiers.
enum class EMouseButton : uint8_t
{
    Left,
    Right,
    Middle,
    Thumb1,
    Thumb2
};

/// Abstract message handler interface.
class APPLICATIONCORE_API FGenericApplicationMessageHandler
{
public:
    virtual ~FGenericApplicationMessageHandler() = default;

    // -- Keyboard --

    /// Called when a key is pressed.
    virtual bool OnKeyDown(int32_t keyCode, uint32_t charCode, bool bIsRepeat)
    {
        return false;
    }

    /// Called when a key is released.
    virtual bool OnKeyUp(int32_t keyCode, uint32_t charCode, bool bIsRepeat)
    {
        return false;
    }

    /// Called when a character is typed.
    virtual bool OnKeyChar(uint32_t character, bool bIsRepeat)
    {
        return false;
    }

    // -- Mouse --

    /// Called when a mouse button is pressed.
    virtual bool OnMouseDown(EMouseButton button, float cursorX, float cursorY)
    {
        return false;
    }

    /// Called when a mouse button is released.
    virtual bool OnMouseUp(EMouseButton button, float cursorX, float cursorY)
    {
        return false;
    }

    /// Called when the mouse is moved.
    virtual bool OnMouseMove(float cursorX, float cursorY)
    {
        return false;
    }

    /// Called when the mouse wheel is scrolled.
    virtual bool OnMouseWheel(float delta, float cursorX, float cursorY)
    {
        return false;
    }

    // -- Window --

    /// Called when a window is resized.
    virtual void OnWindowResized(FGenericWindow* window, int32_t width, int32_t height)
    {
    }

    /// Called when a window gains or loses focus.
    virtual void OnWindowFocusChanged(FGenericWindow* window, bool bHasFocus)
    {
    }

    /// Called when a window close is requested.
    /// @return true to allow close, false to prevent it.
    virtual bool OnWindowCloseRequested(FGenericWindow* window)
    {
        return true;
    }
};

} // namespace Enigma
