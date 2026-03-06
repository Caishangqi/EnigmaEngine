// Copyright EnigmaEngine. All Rights Reserved.

#include "Windows/ConsoleWindow.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "Misc/AssertionMacros.h"

#include <windows.h>
#include <cstdio>

namespace Enigma
{

// ---------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------

FConsoleWindow::FConsoleWindow(const FConsoleWindowSettings& settings)
    : m_settings(settings)
{
    // Acquire console HWND. If no console is attached, allocate one.
    // If the process inherited a headless console (e.g. output redirected
    // by a parent process), GetConsoleWindow() returns nullptr even though
    // a console exists. In that case, detach first, then allocate fresh.
    m_consoleHwnd = ::GetConsoleWindow();
    if (m_consoleHwnd == nullptr)
    {
        ::FreeConsole();
        ::AllocConsole();
        m_consoleHwnd = ::GetConsoleWindow();
    }
    checkf(m_consoleHwnd != nullptr,
        "Failed to acquire console HWND after AllocConsole()");

    // After FreeConsole+AllocConsole, GetStdHandle may return stale handles.
    // Use CreateFileA on CONOUT$/CONIN$ to guarantee fresh handles.
    m_outputHandle = ::CreateFileA("CONOUT$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr);
    m_inputHandle = ::CreateFileA("CONIN$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, 0, nullptr);
    checkf(m_outputHandle != INVALID_HANDLE_VALUE,
        "Failed to acquire console output handle");
    checkf(m_inputHandle != INVALID_HANDLE_VALUE,
        "Failed to acquire console input handle");

    saveOriginalSettings();
    applyConsoleMode();
    applyFont();
    applyBufferSize();
    applyWindowStyle();

    if (m_settings.bRenderFriendly)
    {
        SetRenderFriendly(true);
    }
}

FConsoleWindow::~FConsoleWindow()
{
    if (!m_bDestroyed)
    {
        Destroy();
    }

    // Close handles opened via CreateFileA (not GetStdHandle).
    if (m_outputHandle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_outputHandle);
        m_outputHandle = INVALID_HANDLE_VALUE;
    }
    if (m_inputHandle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_inputHandle);
        m_inputHandle = INVALID_HANDLE_VALUE;
    }
}

// ---------------------------------------------------------------
// FGenericWindow overrides
// ---------------------------------------------------------------

void FConsoleWindow::Show()
{
    if (m_consoleHwnd != nullptr)
    {
        ::ShowWindow(m_consoleHwnd, SW_SHOW);
        m_bVisible = true;
    }
}

void FConsoleWindow::Hide()
{
    if (m_consoleHwnd != nullptr)
    {
        ::ShowWindow(m_consoleHwnd, SW_HIDE);
        m_bVisible = false;
    }
}

void FConsoleWindow::Destroy()
{
    if (m_bDestroyed)
    {
        return;
    }

    restoreOriginalSettings();

    // Do NOT close the console process or free the console.
    // Just mark as destroyed so we stop pumping input.
    m_bDestroyed = true;
}

void FConsoleWindow::Minimize()
{
    if (m_consoleHwnd != nullptr)
    {
        ::ShowWindow(m_consoleHwnd, SW_MINIMIZE);
    }
}

void FConsoleWindow::Maximize()
{
    if (m_consoleHwnd != nullptr)
    {
        ::ShowWindow(m_consoleHwnd, SW_MAXIMIZE);
    }
}

void FConsoleWindow::Restore()
{
    if (m_consoleHwnd != nullptr)
    {
        ::ShowWindow(m_consoleHwnd, SW_RESTORE);
    }
}

void FConsoleWindow::SetTitle(const char* title)
{
    if (m_consoleHwnd != nullptr)
    {
        ::SetConsoleTitleA(title);
    }
}

void FConsoleWindow::Resize(int32_t columns, int32_t rows)
{
    m_settings.Columns = static_cast<int16_t>(columns);
    m_settings.Rows = static_cast<int16_t>(rows);
    applyBufferSize();
}

void FConsoleWindow::Move(int32_t x, int32_t y)
{
    if (m_consoleHwnd != nullptr)
    {
        ::SetWindowPos(
            m_consoleHwnd, nullptr,
            x, y, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void* FConsoleWindow::GetNativeHandle() const
{
    return static_cast<void*>(m_consoleHwnd);
}

bool FConsoleWindow::IsVisible() const
{
    return m_bVisible;
}

bool FConsoleWindow::HasFocus() const
{
    if (m_consoleHwnd != nullptr)
    {
        return ::GetForegroundWindow() == m_consoleHwnd;
    }
    return false;
}

int32_t FConsoleWindow::GetWidth() const
{
    return static_cast<int32_t>(m_settings.Columns);
}

int32_t FConsoleWindow::GetHeight() const
{
    return static_cast<int32_t>(m_settings.Rows);
}

// ---------------------------------------------------------------
// Console-specific public methods
// ---------------------------------------------------------------

void FConsoleWindow::ClearBuffer()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!::GetConsoleScreenBufferInfo(m_outputHandle, &csbi))
    {
        return;
    }

    DWORD bufferSize = static_cast<DWORD>(csbi.dwSize.X) * csbi.dwSize.Y;
    COORD origin = {0, 0};
    DWORD charsWritten = 0;

    ::FillConsoleOutputCharacterA(m_outputHandle, ' ', bufferSize, origin, &charsWritten);
    ::FillConsoleOutputAttribute(m_outputHandle, csbi.wAttributes, bufferSize, origin, &charsWritten);
    ::SetConsoleCursorPosition(m_outputHandle, origin);
}

bool FConsoleWindow::SetFont(const char* fontName, int16_t fontSize)
{
    // Save current font in case we need to restore on failure
    CONSOLE_FONT_INFOEX previousFont = {};
    previousFont.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    ::GetCurrentConsoleFontEx(m_outputHandle, FALSE, &previousFont);

    CONSOLE_FONT_INFOEX fontInfo = {};
    fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    fontInfo.dwFontSize.Y = static_cast<SHORT>(fontSize);
    fontInfo.FontWeight = FW_NORMAL;
    fontInfo.FontFamily = FF_DONTCARE | TMPF_TRUETYPE;

    // Copy font name (wchar_t face name, max 32 chars including null)
    size_t i = 0;
    while (fontName[i] != '\0' && i < LF_FACESIZE - 1)
    {
        fontInfo.FaceName[i] = static_cast<wchar_t>(fontName[i]);
        ++i;
    }
    fontInfo.FaceName[i] = L'\0';

    if (!ensuref(::SetCurrentConsoleFontEx(m_outputHandle, FALSE, &fontInfo),
        "Failed to apply font '{}' size {}", fontName, fontSize))
    {
        // Restore previous font
        ::SetCurrentConsoleFontEx(m_outputHandle, FALSE, &previousFont);
        return false;
    }

    m_settings.FontName = fontName;
    m_settings.FontSize = fontSize;
    return true;
}

void FConsoleWindow::SetRenderFriendly(bool bEnable)
{
    if (bEnable)
    {
        // Hide cursor
        CONSOLE_CURSOR_INFO cursorInfo;
        ::GetConsoleCursorInfo(m_outputHandle, &cursorInfo);
        cursorInfo.bVisible = FALSE;
        ::SetConsoleCursorInfo(m_outputHandle, &cursorInfo);

        // Disable Quick Edit and enable mouse input
        DWORD inputMode = 0;
        ::GetConsoleMode(m_inputHandle, &inputMode);
        inputMode &= ~ENABLE_QUICK_EDIT_MODE;
        inputMode |= ENABLE_EXTENDED_FLAGS;
        if (m_settings.bEnableMouseInput)
        {
            inputMode |= ENABLE_MOUSE_INPUT;
        }
        ::SetConsoleMode(m_inputHandle, inputMode);
    }
    else
    {
        // Restore cursor visibility
        CONSOLE_CURSOR_INFO cursorInfo;
        ::GetConsoleCursorInfo(m_outputHandle, &cursorInfo);
        cursorInfo.bVisible = TRUE;
        ::SetConsoleCursorInfo(m_outputHandle, &cursorInfo);

        // Re-enable Quick Edit
        DWORD inputMode = 0;
        ::GetConsoleMode(m_inputHandle, &inputMode);
        inputMode |= ENABLE_QUICK_EDIT_MODE;
        inputMode |= ENABLE_EXTENDED_FLAGS;
        ::SetConsoleMode(m_inputHandle, inputMode);
    }

    m_settings.bRenderFriendly = bEnable;
}

void FConsoleWindow::ApplySettings(const FConsoleWindowSettings& settings)
{
    m_settings = settings;
    applyBufferSize();
    applyFont();
    applyConsoleMode();
    applyWindowStyle();

    if (m_settings.bRenderFriendly)
    {
        SetRenderFriendly(true);
    }
}

const FConsoleWindowSettings& FConsoleWindow::GetSettings() const
{
    return m_settings;
}

bool FConsoleWindow::SupportsVirtualTerminal() const
{
    return m_bVirtualTerminalSupported;
}

// ---------------------------------------------------------------
// Input pump (called by FWindowsApplication::PumpMessages)
// ---------------------------------------------------------------

void FConsoleWindow::pumpConsoleInput(FGenericApplicationMessageHandler* handler)
{
    if (m_bDestroyed || handler == nullptr)
    {
        return;
    }

    DWORD numEvents = 0;
    ::GetNumberOfConsoleInputEvents(m_inputHandle, &numEvents);
    if (numEvents == 0)
    {
        return;
    }

    // Read all pending input records (non-blocking since we checked count)
    constexpr DWORD kMaxEvents = 128;
    INPUT_RECORD records[kMaxEvents];
    DWORD eventsRead = 0;
    ::ReadConsoleInputA(m_inputHandle, records, kMaxEvents, &eventsRead);

    for (DWORD i = 0; i < eventsRead; ++i)
    {
        const INPUT_RECORD& record = records[i];

        switch (record.EventType)
        {
        case KEY_EVENT:
        {
            const KEY_EVENT_RECORD& key = record.Event.KeyEvent;
            int32_t keyCode = static_cast<int32_t>(key.wVirtualKeyCode);
            uint32_t charCode = static_cast<uint32_t>(key.uChar.UnicodeChar);
            bool bIsRepeat = key.wRepeatCount > 1;

            if (key.bKeyDown)
            {
                handler->OnKeyDown(keyCode, charCode, bIsRepeat);
                if (charCode != 0)
                {
                    handler->OnKeyChar(charCode, bIsRepeat);
                }
            }
            else
            {
                handler->OnKeyUp(keyCode, charCode, bIsRepeat);
            }
            break;
        }
        case MOUSE_EVENT:
        {
            const MOUSE_EVENT_RECORD& mouse = record.Event.MouseEvent;
            float cursorX = static_cast<float>(mouse.dwMousePosition.X);
            float cursorY = static_cast<float>(mouse.dwMousePosition.Y);

            if (mouse.dwEventFlags == 0)
            {
                // Button press/release
                if (mouse.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
                {
                    handler->OnMouseDown(EMouseButton::Left, cursorX, cursorY);
                }
                else
                {
                    handler->OnMouseUp(EMouseButton::Left, cursorX, cursorY);
                }

                if (mouse.dwButtonState & RIGHTMOST_BUTTON_PRESSED)
                {
                    handler->OnMouseDown(EMouseButton::Right, cursorX, cursorY);
                }
                else
                {
                    handler->OnMouseUp(EMouseButton::Right, cursorX, cursorY);
                }
            }
            else if (mouse.dwEventFlags & MOUSE_MOVED)
            {
                handler->OnMouseMove(cursorX, cursorY);
            }
            else if (mouse.dwEventFlags & MOUSE_WHEELED)
            {
                float delta = static_cast<float>(
                    static_cast<short>(HIWORD(mouse.dwButtonState)));
                handler->OnMouseWheel(delta, cursorX, cursorY);
            }
            break;
        }
        case WINDOW_BUFFER_SIZE_EVENT:
        {
            const WINDOW_BUFFER_SIZE_RECORD& size = record.Event.WindowBufferSizeEvent;
            m_settings.Columns = size.dwSize.X;
            m_settings.Rows = size.dwSize.Y;
            handler->OnWindowResized(
                this,
                static_cast<int32_t>(size.dwSize.X),
                static_cast<int32_t>(size.dwSize.Y));
            break;
        }
        default:
            break;
        }
    }
}

// ---------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------

HWND FConsoleWindow::findTopLevelHwnd() const
{
    HWND hwnd = m_consoleHwnd;
    for (HWND parent = ::GetParent(hwnd);
         parent != nullptr;
         parent = ::GetParent(hwnd))
    {
        hwnd = parent;
    }
    return hwnd;
}

void FConsoleWindow::saveOriginalSettings()
{
    // Save output mode
    ::GetConsoleMode(m_outputHandle, &m_originalOutputMode);

    // Save input mode
    ::GetConsoleMode(m_inputHandle, &m_originalInputMode);

    // Save cursor info
    static_assert(sizeof(CONSOLE_CURSOR_INFO) <= sizeof(m_originalCursorInfo),
        "CONSOLE_CURSOR_INFO exceeds opaque storage");
    ::GetConsoleCursorInfo(m_outputHandle,
        reinterpret_cast<CONSOLE_CURSOR_INFO*>(m_originalCursorInfo));

    // Save font info
    static_assert(sizeof(CONSOLE_FONT_INFOEX) <= sizeof(m_originalFontInfo),
        "CONSOLE_FONT_INFOEX exceeds opaque storage");
    auto* fontInfo = reinterpret_cast<CONSOLE_FONT_INFOEX*>(m_originalFontInfo);
    fontInfo->cbSize = sizeof(CONSOLE_FONT_INFOEX);
    ::GetCurrentConsoleFontEx(m_outputHandle, FALSE, fontInfo);

    // Save buffer size
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (::GetConsoleScreenBufferInfo(m_outputHandle, &csbi))
    {
        static_assert(sizeof(COORD) <= sizeof(m_originalBufferSize),
            "COORD exceeds opaque storage");
        *reinterpret_cast<COORD*>(m_originalBufferSize) = csbi.dwSize;
    }

    // Save window style (from top-level window for third-party terminal compat)
    m_originalWindowStyle = ::GetWindowLongA(findTopLevelHwnd(), GWL_STYLE);
}

void FConsoleWindow::restoreOriginalSettings()
{
    // Restore output mode
    ensure(::SetConsoleMode(m_outputHandle, m_originalOutputMode));

    // Restore input mode
    ensure(::SetConsoleMode(m_inputHandle, m_originalInputMode));

    // Restore cursor info
    ensure(::SetConsoleCursorInfo(m_outputHandle,
        reinterpret_cast<const CONSOLE_CURSOR_INFO*>(m_originalCursorInfo)));

    // Restore font
    ensure(::SetCurrentConsoleFontEx(m_outputHandle, FALSE,
        reinterpret_cast<CONSOLE_FONT_INFOEX*>(m_originalFontInfo)));

    // Restore buffer size
    ensure(::SetConsoleScreenBufferSize(m_outputHandle,
        *reinterpret_cast<const COORD*>(m_originalBufferSize)));

    // Restore window style (to top-level window for third-party terminal compat)
    HWND topLevel = findTopLevelHwnd();
    ::SetWindowLongA(topLevel, GWL_STYLE, m_originalWindowStyle);
    ::SetWindowPos(topLevel, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void FConsoleWindow::applyConsoleMode()
{
    // Output mode: enable Virtual Terminal Processing if requested
    DWORD outputMode = 0;
    ::GetConsoleMode(m_outputHandle, &outputMode);
    outputMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;

    if (m_settings.bEnableVirtualTerminal)
    {
        outputMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        BOOL result = ::SetConsoleMode(m_outputHandle, outputMode);
        m_bVirtualTerminalSupported = (result != FALSE);

        if (!m_bVirtualTerminalSupported)
        {
            // Fall back without VT processing
            outputMode &= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            ensure(::SetConsoleMode(m_outputHandle, outputMode));
        }
    }
    else
    {
        verify(::SetConsoleMode(m_outputHandle, outputMode));
        m_bVirtualTerminalSupported = false;
    }

    // Input mode: enable mouse input if requested
    DWORD inputMode = ENABLE_EXTENDED_FLAGS;
    if (m_settings.bEnableMouseInput)
    {
        inputMode |= ENABLE_MOUSE_INPUT;
    }
    verify(::SetConsoleMode(m_inputHandle, inputMode));
}

void FConsoleWindow::applyFont()
{
    CONSOLE_FONT_INFOEX fontInfo = {};
    fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    fontInfo.dwFontSize.Y = static_cast<SHORT>(m_settings.FontSize);
    fontInfo.FontWeight = FW_NORMAL;
    fontInfo.FontFamily = FF_DONTCARE | TMPF_TRUETYPE;

    // Copy font name to wide string
    const char* name = m_settings.FontName;
    size_t i = 0;
    while (name[i] != '\0' && i < LF_FACESIZE - 1)
    {
        fontInfo.FaceName[i] = static_cast<wchar_t>(name[i]);
        ++i;
    }
    fontInfo.FaceName[i] = L'\0';

    if (!ensure(::SetCurrentConsoleFontEx(m_outputHandle, FALSE, &fontInfo)))
    {
        // Font application failed; keep whatever font is currently active
    }
}

void FConsoleWindow::applyBufferSize()
{
    COORD bufferSize;
    bufferSize.X = m_settings.Columns;
    bufferSize.Y = m_settings.Rows;

    // To avoid errors, set the window size smaller first if needed,
    // then set buffer, then set window to match.
    SMALL_RECT minWindow = {0, 0, 1, 1};
    ::SetConsoleWindowInfo(m_outputHandle, TRUE, &minWindow);

    verify(::SetConsoleScreenBufferSize(m_outputHandle, bufferSize));

    SMALL_RECT windowRect;
    windowRect.Left = 0;
    windowRect.Top = 0;
    windowRect.Right = m_settings.Columns - 1;
    windowRect.Bottom = m_settings.Rows - 1;
    verify(::SetConsoleWindowInfo(m_outputHandle, TRUE, &windowRect));

    // Verify the window size was actually applied.
    // Third-party terminals (Cmder/ConEmu, Windows Terminal) may ignore
    // SetConsoleWindowInfo. Fall back to the xterm resize escape sequence
    // ESC[8;rows;cols t which is widely supported by modern terminals.
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (m_bVirtualTerminalSupported
        && ::GetConsoleScreenBufferInfo(m_outputHandle, &csbi))
    {
        SHORT actualW = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        SHORT actualH = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        if (actualW != m_settings.Columns || actualH != m_settings.Rows)
        {
            char buf[32];
            int len = std::snprintf(buf, sizeof(buf), "\x1b[8;%d;%dt",
                static_cast<int>(m_settings.Rows),
                static_cast<int>(m_settings.Columns));
            DWORD written = 0;
            ::WriteConsoleA(m_outputHandle, buf, static_cast<DWORD>(len),
                &written, nullptr);
        }
    }
}

void FConsoleWindow::applyWindowStyle()
{
    if (m_consoleHwnd == nullptr)
    {
        return;
    }

    HWND targetHwnd = findTopLevelHwnd();

    LONG style = ::GetWindowLongA(targetHwnd, GWL_STYLE);

    if (!m_settings.bResizable)
    {
        style &= ~WS_SIZEBOX;
        style &= ~WS_MAXIMIZEBOX;
    }

    if (!m_settings.bShowScrollBar)
    {
        style &= ~WS_VSCROLL;
        style &= ~WS_HSCROLL;
    }

    ::SetWindowLongA(targetHwnd, GWL_STYLE, style);
    ::SetWindowPos(targetHwnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

} // namespace Enigma
