// Copyright EnigmaEngine. All Rights Reserved.

#include "Windows/WindowsWindow.h"
#include "Misc/AssertionMacros.h"

namespace Enigma
{

FWindowsWindow::~FWindowsWindow()
{
    if (m_hwnd != nullptr)
    {
        Destroy();
    }
}

bool FWindowsWindow::Initialize(
    const FWindowDefinition& definition,
    HINSTANCE hInstance,
    const char* className,
    void* lpParam)
{
    // Build window style
    m_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    if (definition.bHasOSWindowBorder)
    {
        m_style |= WS_BORDER;
    }

    if (definition.bIsResizable)
    {
        m_style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
    }

    // Adjust window rect for non-client area
    RECT windowRect = { 0, 0, definition.Width, definition.Height };
    ::AdjustWindowRect(&windowRect, m_style, FALSE);

    int adjustedWidth = windowRect.right - windowRect.left;
    int adjustedHeight = windowRect.bottom - windowRect.top;

    // Position: -1 means OS default
    int posX = (definition.PositionX == -1) ? CW_USEDEFAULT : definition.PositionX;
    int posY = (definition.PositionY == -1) ? CW_USEDEFAULT : definition.PositionY;

    m_hwnd = ::CreateWindowExA(
        0,                      // dwExStyle
        className,              // lpClassName
        definition.Title,       // lpWindowName
        m_style,                // dwStyle
        posX,                   // X
        posY,                   // Y
        adjustedWidth,          // nWidth
        adjustedHeight,         // nHeight
        nullptr,                // hWndParent
        nullptr,                // hMenu
        hInstance,              // hInstance
        lpParam);               // lpParam (passed to WM_CREATE)

    if (!ensuref(m_hwnd != nullptr, "CreateWindowEx failed: {}", ::GetLastError()))
    {
        return false;
    }

    m_width = definition.Width;
    m_height = definition.Height;

    if (definition.bShowOnCreation)
    {
        Show();
    }

    return true;
}

void FWindowsWindow::Show()
{
    if (m_hwnd != nullptr)
    {
        ::ShowWindow(m_hwnd, SW_SHOW);
    }
}

void FWindowsWindow::Hide()
{
    if (m_hwnd != nullptr)
    {
        ::ShowWindow(m_hwnd, SW_HIDE);
    }
}

void FWindowsWindow::Destroy()
{
    if (m_hwnd != nullptr)
    {
        ::DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void FWindowsWindow::Minimize()
{
    if (m_hwnd != nullptr)
    {
        ::ShowWindow(m_hwnd, SW_MINIMIZE);
    }
}

void FWindowsWindow::Maximize()
{
    if (m_hwnd != nullptr)
    {
        ::ShowWindow(m_hwnd, SW_MAXIMIZE);
    }
}

void FWindowsWindow::Restore()
{
    if (m_hwnd != nullptr)
    {
        ::ShowWindow(m_hwnd, SW_RESTORE);
    }
}

void FWindowsWindow::SetTitle(const char* title)
{
    if (m_hwnd != nullptr)
    {
        ::SetWindowTextA(m_hwnd, title);
    }
}

void FWindowsWindow::Resize(int32_t width, int32_t height)
{
    if (m_hwnd != nullptr)
    {
        // Adjust for non-client area
        RECT windowRect = { 0, 0, width, height };
        ::AdjustWindowRect(&windowRect, m_style, FALSE);

        int adjustedWidth = windowRect.right - windowRect.left;
        int adjustedHeight = windowRect.bottom - windowRect.top;

        ::SetWindowPos(
            m_hwnd, nullptr,
            0, 0,
            adjustedWidth, adjustedHeight,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        m_width = width;
        m_height = height;
    }
}

void FWindowsWindow::Move(int32_t x, int32_t y)
{
    if (m_hwnd != nullptr)
    {
        ::SetWindowPos(
            m_hwnd, nullptr,
            x, y,
            0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void* FWindowsWindow::GetNativeHandle() const
{
    return static_cast<void*>(m_hwnd);
}

bool FWindowsWindow::IsVisible() const
{
    if (m_hwnd != nullptr)
    {
        return ::IsWindowVisible(m_hwnd) != FALSE;
    }
    return false;
}

bool FWindowsWindow::HasFocus() const
{
    if (m_hwnd != nullptr)
    {
        return ::GetForegroundWindow() == m_hwnd;
    }
    return false;
}

int32_t FWindowsWindow::GetWidth() const
{
    return m_width;
}

int32_t FWindowsWindow::GetHeight() const
{
    return m_height;
}

void FWindowsWindow::OnResized(int32_t newWidth, int32_t newHeight)
{
    m_width = newWidth;
    m_height = newHeight;
}

} // namespace Enigma
