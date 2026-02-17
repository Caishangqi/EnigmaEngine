// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "ApplicationCoreAPI.generated.h"

#include <cstdint>

// -------------------------------------------------------------
// FWindowDefinition
//
// Value type describing window creation parameters.
// Passed to FGenericApplication::MakeWindow().
// -------------------------------------------------------------

namespace Enigma
{

/// Window creation parameters.
struct APPLICATIONCORE_API FWindowDefinition
{
    /// Window title bar text.
    const char* Title = "Enigma Engine";

    /// Client area width in pixels.
    int32_t Width = 1280;

    /// Client area height in pixels.
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
};

} // namespace Enigma
