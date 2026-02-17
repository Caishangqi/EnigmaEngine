// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericApplication.h"
#include "Windows/WindowsWindow.h"

#include <windows.h>

#include <cstdint>
#include <memory>
#include <vector>

// -------------------------------------------------------------
// FWindowsApplication
//
// Win32 application implementation owning the WndProc, message
// pump, and window factory. Created by
// FGenericApplication::CreateApplication() on Windows.
// -------------------------------------------------------------

namespace Enigma
{

class FWindowsApplication : public FGenericApplication
{
public:
    FWindowsApplication();
    ~FWindowsApplication() override;

    // -- FGenericApplication overrides --
    void PumpMessages(float deltaTime) override;
    FGenericWindow* MakeWindow(const FWindowDefinition& definition) override;
    void DestroyWindow(FGenericWindow* window) override;

    /// Static WndProc callback registered with the Win32 window class.
    static LRESULT CALLBACK AppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    /// Translate a Win32 message to MessageHandler callbacks.
    void ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /// Find the FWindowsWindow associated with an HWND.
    FWindowsWindow* FindWindowByHwnd(HWND hwnd) const;

    /// Register the Win32 window class (called once in constructor).
    void RegisterWindowClass();

    HINSTANCE m_hInstance = nullptr;
    std::vector<std::unique_ptr<FWindowsWindow>> m_windows;

    /// Class name for RegisterClassEx.
    static constexpr const char* kWindowClassName = "EnigmaEngineWindow";
    static bool s_windowClassRegistered;
};

} // namespace Enigma
