// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericWindow.h"
#include "GenericPlatform/ConsoleWindowSettings.h"

#include <cstdint>

// Forward-declare Win32 types to avoid leaking <windows.h> into headers.
// The actual Win32 includes are in ConsoleWindow.cpp only.
using HANDLE = void*;
using HWND = struct HWND__*;
using DWORD = unsigned long;
using LONG = long;
using WORD = unsigned short;
using SHORT = short;

// Win32 COORD and CONSOLE_CURSOR_INFO forward declarations
// are not possible (they are structs), so we store them as
// opaque byte arrays sized to match the Win32 originals.

// -------------------------------------------------------------
// FConsoleWindow
//
// FGenericWindow subclass wrapping the Win32 console HWND.
// Provides console size control, font configuration,
// render-friendly mode, input routing, and ANSI color support.
//
// Created by FWindowsApplication::MakeWindow() when
// FWindowDefinition::Type == EWindowType::Console.
// -------------------------------------------------------------

namespace Enigma
{

class FGenericApplicationMessageHandler;

/// Console window wrapping the Win32 console HWND.
class APPLICATIONCORE_API FConsoleWindow : public FGenericWindow
{
public:
    explicit FConsoleWindow(const FConsoleWindowSettings& settings);
    ~FConsoleWindow() override;

    // -- FGenericWindow overrides (all 14 pure virtuals) --
    void Show() override;
    void Hide() override;
    void Destroy() override;
    void Minimize() override;
    void Maximize() override;
    void Restore() override;
    void SetTitle(const char* title) override;
    void Resize(int32_t columns, int32_t rows) override;
    void Move(int32_t x, int32_t y) override;
    void* GetNativeHandle() const override;
    bool IsVisible() const override;
    bool HasFocus() const override;
    int32_t GetWidth() const override;   // returns columns
    int32_t GetHeight() const override;  // returns rows

    // -- Console-specific public methods --

    /// Clear the entire console screen buffer and reset cursor to (0,0).
    void ClearBuffer();

    /// Set the console font. Returns false if the font is unavailable.
    bool SetFont(const char* fontName, int16_t fontSize);

    /// Toggle render-friendly mode (hide cursor, disable Quick Edit, etc.).
    void SetRenderFriendly(bool bEnable);

    /// Apply a full settings struct at runtime.
    void ApplySettings(const FConsoleWindowSettings& settings);

    /// Get the current effective settings.
    const FConsoleWindowSettings& GetSettings() const;

    /// Whether ANSI Virtual Terminal Processing is supported.
    bool SupportsVirtualTerminal() const;

    // -- Internal: called by FWindowsApplication --

    /// Pump console input events and route to the message handler.
    void pumpConsoleInput(FGenericApplicationMessageHandler* handler);

private:
    void applyConsoleMode();
    void applyFont();
    void applyBufferSize();
    void applyWindowStyle();
    void saveOriginalSettings();
    void restoreOriginalSettings();

    /// Walk up the parent chain from m_consoleHwnd to find the actual
    /// top-level window. Third-party terminals (Cmder/ConEmu, Windows
    /// Terminal) wrap conhost in their own window; style changes must
    /// target the top-level window to be visible.
    HWND findTopLevelHwnd() const;

    HANDLE m_outputHandle = reinterpret_cast<HANDLE>(static_cast<intptr_t>(-1));
    HANDLE m_inputHandle = reinterpret_cast<HANDLE>(static_cast<intptr_t>(-1));
    HWND m_consoleHwnd = nullptr;
    FConsoleWindowSettings m_settings;

    // Saved originals for restoration on Destroy()
    DWORD m_originalOutputMode = 0;
    DWORD m_originalInputMode = 0;

    // Opaque storage for Win32 CONSOLE_CURSOR_INFO (8 bytes)
    alignas(4) unsigned char m_originalCursorInfo[8] = {};

    // Opaque storage for Win32 CONSOLE_FONT_INFOEX (84 bytes)
    alignas(4) unsigned char m_originalFontInfo[84] = {};

    // Opaque storage for Win32 COORD (4 bytes)
    alignas(2) unsigned char m_originalBufferSize[4] = {};

    LONG m_originalWindowStyle = 0;

    bool m_bVirtualTerminalSupported = false;
    bool m_bVisible = true;
    bool m_bDestroyed = false;
};

} // namespace Enigma
