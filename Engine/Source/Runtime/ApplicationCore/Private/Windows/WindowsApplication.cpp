// Copyright EnigmaEngine. All Rights Reserved.

#include "Windows/WindowsApplication.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "GenericPlatform/ConsoleWindowSettings.h"
#include "Misc/AssertionMacros.h"

#include <windowsx.h>

namespace Enigma
{

bool FWindowsApplication::s_windowClassRegistered = false;

// -------------------------------------------------------------
// Construction / Destruction
// -------------------------------------------------------------

FWindowsApplication::FWindowsApplication()
{
    m_hInstance = ::GetModuleHandle(nullptr);
    RegisterWindowClass();
}

FWindowsApplication::~FWindowsApplication()
{
    m_windows.clear();

    if (s_windowClassRegistered)
    {
        ::UnregisterClassA(kWindowClassName, m_hInstance);
        s_windowClassRegistered = false;
    }
}

// -------------------------------------------------------------
// Window Class Registration
// -------------------------------------------------------------

void FWindowsApplication::RegisterWindowClass()
{
    if (s_windowClassRegistered)
    {
        return;
    }

    WNDCLASSEXA wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = &FWindowsApplication::AppWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = m_hInstance;
    wc.hIcon         = ::LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = kWindowClassName;
    wc.hIconSm      = ::LoadIcon(nullptr, IDI_APPLICATION);

    ATOM result = ::RegisterClassExA(&wc);
    checkf(result != 0, "RegisterClassEx failed: {}", ::GetLastError());

    s_windowClassRegistered = true;
}

// -------------------------------------------------------------
// AppWndProc (static)
// -------------------------------------------------------------

LRESULT CALLBACK FWindowsApplication::AppWndProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    FWindowsApplication* app = nullptr;

    if (msg == WM_CREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = static_cast<FWindowsApplication*>(cs->lpCreateParams);
        ::SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else
    {
        app = reinterpret_cast<FWindowsApplication*>(
            ::GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    }

    if (app != nullptr)
    {
        app->ProcessMessage(hwnd, msg, wParam, lParam);

        // WM_CLOSE is fully handled by ProcessMessage (which respects
        // OnWindowCloseRequested). Do not pass to DefWindowProc, which
        // would unconditionally call ::DestroyWindow.
        if (msg == WM_CLOSE)
        {
            return 0;
        }
    }

    return ::DefWindowProcA(hwnd, msg, wParam, lParam);
}

// -------------------------------------------------------------
// PumpMessages
// -------------------------------------------------------------

void FWindowsApplication::PumpMessages(float /*deltaTime*/)
{
    MSG msg = {};
    while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            // Signal engine to exit via RequestEngineExit or similar.
            // For now, we rely on the message handler / engine loop
            // detecting that all windows are closed.
            break;
        }

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    // Pump console input for any console windows.
    FGenericApplicationMessageHandler* handler = GetMessageHandler();
    if (handler != nullptr)
    {
        for (const auto& window : m_windows)
        {
            auto* consoleWindow = dynamic_cast<FConsoleWindow*>(window.get());
            if (consoleWindow != nullptr)
            {
                consoleWindow->pumpConsoleInput(handler);
            }
        }
    }
}

// -------------------------------------------------------------
// ProcessMessage
// -------------------------------------------------------------

void FWindowsApplication::ProcessMessage(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    FGenericApplicationMessageHandler* handler = GetMessageHandler();

    switch (msg)
    {
    // -- Keyboard --

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        if (handler != nullptr)
        {
            int32_t keyCode = static_cast<int32_t>(wParam);
            uint32_t charCode = static_cast<uint32_t>(wParam);
            bool bIsRepeat = (lParam & 0x40000000) != 0;
            handler->OnKeyDown(keyCode, charCode, bIsRepeat);
        }
        break;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        if (handler != nullptr)
        {
            int32_t keyCode = static_cast<int32_t>(wParam);
            uint32_t charCode = static_cast<uint32_t>(wParam);
            bool bIsRepeat = false;
            handler->OnKeyUp(keyCode, charCode, bIsRepeat);
        }
        break;
    }

    case WM_CHAR:
    {
        if (handler != nullptr)
        {
            uint32_t character = static_cast<uint32_t>(wParam);
            bool bIsRepeat = (lParam & 0x40000000) != 0;
            handler->OnKeyChar(character, bIsRepeat);
        }
        break;
    }

    // -- Mouse Buttons --

    case WM_LBUTTONDOWN:
    {
        if (handler != nullptr)
        {
            float cursorX = static_cast<float>(GET_X_LPARAM(lParam));
            float cursorY = static_cast<float>(GET_Y_LPARAM(lParam));
            handler->OnMouseDown(EMouseButton::Left, cursorX, cursorY);
        }
        break;
    }

    case WM_RBUTTONDOWN:
    {
        if (handler != nullptr)
        {
            float cursorX = static_cast<float>(GET_X_LPARAM(lParam));
            float cursorY = static_cast<float>(GET_Y_LPARAM(lParam));
            handler->OnMouseDown(EMouseButton::Right, cursorX, cursorY);
        }
        break;
    }

    case WM_MBUTTONDOWN:
    {
        if (handler != nullptr)
        {
            float cursorX = static_cast<float>(GET_X_LPARAM(lParam));
            float cursorY = static_cast<float>(GET_Y_LPARAM(lParam));
            handler->OnMouseDown(EMouseButton::Middle, cursorX, cursorY);
        }
        break;
    }

    case WM_LBUTTONUP:
    {
        if (handler != nullptr)
        {
            float cursorX = static_cast<float>(GET_X_LPARAM(lParam));
            float cursorY = static_cast<float>(GET_Y_LPARAM(lParam));
            handler->OnMouseUp(EMouseButton::Left, cursorX, cursorY);
        }
        break;
    }

    case WM_RBUTTONUP:
    {
        if (handler != nullptr)
        {
            float cursorX = static_cast<float>(GET_X_LPARAM(lParam));
            float cursorY = static_cast<float>(GET_Y_LPARAM(lParam));
            handler->OnMouseUp(EMouseButton::Right, cursorX, cursorY);
        }
        break;
    }

    case WM_MBUTTONUP:
    {
        if (handler != nullptr)
        {
            float cursorX = static_cast<float>(GET_X_LPARAM(lParam));
            float cursorY = static_cast<float>(GET_Y_LPARAM(lParam));
            handler->OnMouseUp(EMouseButton::Middle, cursorX, cursorY);
        }
        break;
    }

    // -- Mouse Move / Wheel --

    case WM_MOUSEMOVE:
    {
        if (handler != nullptr)
        {
            float cursorX = static_cast<float>(GET_X_LPARAM(lParam));
            float cursorY = static_cast<float>(GET_Y_LPARAM(lParam));
            handler->OnMouseMove(cursorX, cursorY);
        }
        break;
    }

    case WM_MOUSEWHEEL:
    {
        if (handler != nullptr)
        {
            float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) /
                          static_cast<float>(WHEEL_DELTA);
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ::ScreenToClient(hwnd, &pt);
            handler->OnMouseWheel(
                delta,
                static_cast<float>(pt.x),
                static_cast<float>(pt.y));
        }
        break;
    }

    // -- Window Events --

    case WM_SIZE:
    {
        FWindowsWindow* window = FindWindowByHwnd(hwnd);
        if (window != nullptr)
        {
            int32_t width = static_cast<int32_t>(LOWORD(lParam));
            int32_t height = static_cast<int32_t>(HIWORD(lParam));
            window->OnResized(width, height);

            if (handler != nullptr)
            {
                handler->OnWindowResized(window, width, height);
            }
        }
        break;
    }

    case WM_SETFOCUS:
    {
        FWindowsWindow* window = FindWindowByHwnd(hwnd);
        if (handler != nullptr && window != nullptr)
        {
            handler->OnWindowFocusChanged(window, true);
        }
        break;
    }

    case WM_KILLFOCUS:
    {
        FWindowsWindow* window = FindWindowByHwnd(hwnd);
        if (handler != nullptr && window != nullptr)
        {
            handler->OnWindowFocusChanged(window, false);
        }
        break;
    }

    case WM_CLOSE:
    {
        FWindowsWindow* window = FindWindowByHwnd(hwnd);
        if (window != nullptr)
        {
            bool bAllowClose = true;
            if (handler != nullptr)
            {
                bAllowClose = handler->OnWindowCloseRequested(window);
            }

            if (bAllowClose)
            {
                DestroyWindow(window);
            }
        }
        break;
    }

    default:
        break;
    }
}

// -------------------------------------------------------------
// MakeWindow
// -------------------------------------------------------------

FGenericWindow* FWindowsApplication::MakeWindow(const FWindowDefinition& definition)
{
    if (definition.Type == EWindowType::Console)
    {
        FConsoleWindowSettings settings;
        settings.Columns = static_cast<int16_t>(definition.Width);
        settings.Rows    = static_cast<int16_t>(definition.Height);
        settings.bResizable = definition.bIsResizable;

        auto window = std::make_unique<FConsoleWindow>(settings);
        FGenericWindow* rawPtr = window.get();
        m_windows.push_back(std::move(window));
        return rawPtr;
    }

    // Native window path (unchanged)
    auto window = std::make_unique<FWindowsWindow>();

    bool bSuccess = window->Initialize(
        definition,
        m_hInstance,
        kWindowClassName,
        this);

    if (!bSuccess)
    {
        return nullptr;
    }

    FGenericWindow* rawPtr = window.get();
    m_windows.push_back(std::move(window));
    return rawPtr;
}

// -------------------------------------------------------------
// DestroyWindow
// -------------------------------------------------------------

void FWindowsApplication::DestroyWindow(FGenericWindow* window)
{
    for (auto it = m_windows.begin(); it != m_windows.end(); ++it)
    {
        if (it->get() == window)
        {
            (*it)->Destroy();
            m_windows.erase(it);
            return;
        }
    }
}

// -------------------------------------------------------------
// FindWindowByHwnd
// -------------------------------------------------------------

FWindowsWindow* FWindowsApplication::FindWindowByHwnd(HWND hwnd) const
{
    for (const auto& window : m_windows)
    {
        auto* nativeWindow = dynamic_cast<FWindowsWindow*>(window.get());
        if (nativeWindow != nullptr &&
            nativeWindow->GetNativeHandle() == static_cast<void*>(hwnd))
        {
            return nativeWindow;
        }
    }
    return nullptr;
}

} // namespace Enigma
