// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericWindow.h"
#include "GenericPlatform/GenericWindowDefinition.h"

#include <windows.h>

#include <cstdint>

// -------------------------------------------------------------
// FWindowsWindow
//
// Win32 window implementation wrapping HWND.
// Created by FWindowsApplication::MakeWindow().
// -------------------------------------------------------------

namespace Enigma
{

class FWindowsWindow : public FGenericWindow
{
public:
    FWindowsWindow() = default;
    ~FWindowsWindow() override;

    /// Initialize the window with Win32 CreateWindowEx.
    /// @param definition  Window creation parameters.
    /// @param hInstance   Application HINSTANCE.
    /// @param className   Registered window class name.
    /// @param lpParam     Value passed to WM_CREATE (typically FWindowsApplication*).
    /// @return true if window was created successfully.
    bool Initialize(
        const FWindowDefinition& definition,
        HINSTANCE hInstance,
        const char* className,
        void* lpParam);

    // -- FGenericWindow overrides --
    void Show() override;
    void Hide() override;
    void Destroy() override;
    void Minimize() override;
    void Maximize() override;
    void Restore() override;
    void SetTitle(const char* title) override;
    void Resize(int32_t width, int32_t height) override;
    void Move(int32_t x, int32_t y) override;
    void* GetNativeHandle() const override;
    bool IsVisible() const override;
    bool HasFocus() const override;
    int32_t GetWidth() const override;
    int32_t GetHeight() const override;

    /// Update cached dimensions (called by FWindowsApplication on WM_SIZE).
    void OnResized(int32_t newWidth, int32_t newHeight);

private:
    HWND m_hwnd = nullptr;
    DWORD m_style = 0;
    int32_t m_width = 0;
    int32_t m_height = 0;
};

} // namespace Enigma
