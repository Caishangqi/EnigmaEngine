// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "ApplicationCoreAPI.generated.h"

#include <cstdint>

// -------------------------------------------------------------
// EWindowType
//
// Classifies the kind of window to create.
// Used by FGenericApplication::MakeWindow() to dispatch to the
// correct platform window subclass.
// -------------------------------------------------------------

namespace Enigma
{

/// Window type classification for MakeWindow() dispatch.
enum class EWindowType : uint8_t
{
    /// Standard OS window (HWND via CreateWindowEx).
    Native = 0,

    /// Win32 console window (GetConsoleWindow).
    Console = 1,
};

// -------------------------------------------------------------
// FWindowDefinition
//
// Value type describing window creation parameters.
// Passed to FGenericApplication::MakeWindow().
// -------------------------------------------------------------

/// Window creation parameters.
struct APPLICATIONCORE_API FWindowDefinition
{
    /// Window title bar text.
    const char* Title = "Enigma Engine";

    /// Client area width in pixels (Native) or columns (Console).
    int32_t Width = 1280;

    /// Client area height in pixels (Native) or rows (Console).
    int32_t Height = 720;

    /// Window X position (-1 = OS default / centered).
    int32_t PositionX = -1;

    /// Window Y position (-1 = OS default / centered).
    int32_t PositionY = -1;

    /// Whether the window has an OS-drawn border.
    bool bHasOSWindowBorder = true;

    /// Whether the window is resizable by the user.
    bool bIsResizable = true;

    /// Whether to show the window immediately after creation.
    bool bShowOnCreation = true;

    /// Window type: Native (default) or Console.
    EWindowType Type = EWindowType::Native;
};

} // namespace Enigma
