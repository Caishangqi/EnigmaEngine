// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "ApplicationCoreAPI.generated.h"

#include <cstdint>

// -------------------------------------------------------------
// FConsoleWindowSettings
//
// Plain data struct holding all configurable console window
// parameters. Designed for future .ini / .yaml serialization.
//
// All fields have sensible defaults for a typical ASCII game:
//   120x40 buffer, Consolas 16pt, render-friendly mode on.
// -------------------------------------------------------------

namespace Enigma
{

/// Configurable parameters for FConsoleWindow.
struct APPLICATIONCORE_API FConsoleWindowSettings
{
    /// Console buffer width in character columns.
    int16_t Columns = 120;

    /// Console buffer height in character rows.
    int16_t Rows = 40;

    /// Console font face name (must be a monospace font).
    const char* FontName = "Consolas";

    /// Console font size in pixels.
    int16_t FontSize = 16;

    /// Whether the console window can be resized by the user.
    bool bResizable = false;

    /// Enable render-friendly mode: hide cursor, disable Quick Edit,
    /// disable scrollbar, disable text selection.
    bool bRenderFriendly = true;

    /// Enable mouse input events from the console.
    bool bEnableMouseInput = true;

    /// Enable ANSI Virtual Terminal Processing for escape sequences.
    bool bEnableVirtualTerminal = true;

    /// Whether to show scrollbars on the console window.
    bool bShowScrollBar = false;
};

} // namespace Enigma
