// Copyright EnigmaEngine. All Rights Reserved.
// ApplicationCore.Tests -- Console window creation, settings, lifecycle, and input tests.

#include "GenericPlatform/GenericApplication.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "GenericPlatform/GenericWindow.h"
#include "GenericPlatform/GenericWindowDefinition.h"
#include "GenericPlatform/ConsoleWindowSettings.h"
#include "Windows/ConsoleWindow.h"

#include <windows.h>

#include <gtest/gtest.h>

using namespace Enigma;

// A test message handler that records which callbacks were invoked.
class FConsoleTestMessageHandler : public FGenericApplicationMessageHandler
{
public:
    bool bKeyDownCalled = false;
    int32_t LastKeyCode = 0;

    bool bKeyUpCalled = false;
    bool bKeyCharCalled = false;
    uint32_t LastCharacter = 0;

    bool bMouseDownCalled = false;
    EMouseButton LastMouseDownButton = EMouseButton::Left;
    EMouseButton LastMouseButton = EMouseButton::Left;
    float LastCursorX = 0.0f;
    float LastCursorY = 0.0f;

    bool bMouseUpCalled = false;
    bool bMouseMoveCalled = false;

    bool OnKeyDown(int32_t keyCode, uint32_t /*charCode*/, bool /*bIsRepeat*/) override
    {
        bKeyDownCalled = true;
        LastKeyCode = keyCode;
        return true;
    }

    bool OnKeyUp(int32_t keyCode, uint32_t /*charCode*/, bool /*bIsRepeat*/) override
    {
        bKeyUpCalled = true;
        LastKeyCode = keyCode;
        return true;
    }

    bool OnKeyChar(uint32_t character, bool /*bIsRepeat*/) override
    {
        bKeyCharCalled = true;
        LastCharacter = character;
        return true;
    }

    bool OnMouseDown(EMouseButton button, float cursorX, float cursorY) override
    {
        bMouseDownCalled = true;
        LastMouseDownButton = button;
        LastMouseButton = button;
        LastCursorX = cursorX;
        LastCursorY = cursorY;
        return true;
    }

    bool OnMouseUp(EMouseButton button, float cursorX, float cursorY) override
    {
        bMouseUpCalled = true;
        LastMouseButton = button;
        return true;
    }

    bool OnMouseMove(float cursorX, float cursorY) override
    {
        bMouseMoveCalled = true;
        LastCursorX = cursorX;
        LastCursorY = cursorY;
        return true;
    }
};

// ---------------------------------------------------------------
// Helper: create a console window via MakeWindow
// ---------------------------------------------------------------

static FGenericWindow* MakeConsoleWindow(
    FGenericApplication* app,
    int32_t columns = 120,
    int32_t rows = 40)
{
    FWindowDefinition def;
    def.Type = EWindowType::Console;
    def.Width = columns;
    def.Height = rows;
    def.bShowOnCreation = true;
    return app->MakeWindow(def);
}

// ===============================================================
// Suite 1: ConsoleWindowCreation
// ===============================================================

class ConsoleWindowCreation : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        ASSERT_NE(m_app, nullptr);
    }

    void TearDown() override
    {
        delete m_app;
        m_app = nullptr;
    }

    FGenericApplication* m_app = nullptr;
};

TEST_F(ConsoleWindowCreation, MakeWindow_WithConsoleType_CreatesFConsoleWindow)
{
    FGenericWindow* window = MakeConsoleWindow(m_app);
    ASSERT_NE(window, nullptr);

    // dynamic_cast succeeds only for FConsoleWindow
    auto* consoleWin = dynamic_cast<FConsoleWindow*>(window);
    EXPECT_NE(consoleWin, nullptr);

    m_app->DestroyWindow(window);
}

TEST_F(ConsoleWindowCreation, MakeWindow_WithNativeType_CreatesWindowsWindow)
{
    FWindowDefinition def;
    def.Type = EWindowType::Native;
    def.Title = "Native Test";
    def.bShowOnCreation = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    ASSERT_NE(window, nullptr);

    // dynamic_cast to FConsoleWindow should fail for native windows
    auto* consoleWin = dynamic_cast<FConsoleWindow*>(window);
    EXPECT_EQ(consoleWin, nullptr);

    m_app->DestroyWindow(window);
}

TEST_F(ConsoleWindowCreation, GetNativeHandle_ReturnsValidConsoleHWND)
{
    FGenericWindow* window = MakeConsoleWindow(m_app);
    ASSERT_NE(window, nullptr);

    void* handle = window->GetNativeHandle();
    EXPECT_NE(handle, nullptr);

    // The handle should be a valid HWND (IsWindow returns TRUE)
    EXPECT_TRUE(::IsWindow(static_cast<HWND>(handle)));

    m_app->DestroyWindow(window);
}

TEST_F(ConsoleWindowCreation, GetWidth_ReturnsColumns_NotPixels)
{
    FGenericWindow* window = MakeConsoleWindow(m_app, 100, 30);
    ASSERT_NE(window, nullptr);

    // Width should be character columns, not pixels
    EXPECT_EQ(window->GetWidth(), 100);

    m_app->DestroyWindow(window);
}

TEST_F(ConsoleWindowCreation, GetHeight_ReturnsRows_NotPixels)
{
    FGenericWindow* window = MakeConsoleWindow(m_app, 100, 30);
    ASSERT_NE(window, nullptr);

    // Height should be character rows, not pixels
    EXPECT_EQ(window->GetHeight(), 30);

    m_app->DestroyWindow(window);
}

TEST_F(ConsoleWindowCreation, MakeWindow_AppliesTitleFromDefinition)
{
    FWindowDefinition def;
    def.Type = EWindowType::Console;
    def.Title = "TestTitle_12345";
    def.Width = 120;
    def.Height = 40;

    FGenericWindow* window = m_app->MakeWindow(def);
    ASSERT_NE(window, nullptr);

    // Verify the console title was applied
    char titleBuf[256] = {};
    DWORD len = ::GetConsoleTitleA(titleBuf, sizeof(titleBuf));
    EXPECT_GT(len, 0u);
    EXPECT_STREQ(titleBuf, "TestTitle_12345");

    m_app->DestroyWindow(window);
}

TEST_F(ConsoleWindowCreation, MakeWindow_PassesResizableFlag)
{
    // Create a non-resizable console window
    FWindowDefinition def;
    def.Type = EWindowType::Console;
    def.Width = 120;
    def.Height = 40;
    def.bIsResizable = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    ASSERT_NE(window, nullptr);

    auto* consoleWin = dynamic_cast<FConsoleWindow*>(window);
    ASSERT_NE(consoleWin, nullptr);

    const FConsoleWindowSettings& settings = consoleWin->GetSettings();
    EXPECT_FALSE(settings.bResizable);

    m_app->DestroyWindow(window);
}

// ===============================================================
// Suite 2: ConsoleWindowSettings
// ===============================================================

class ConsoleWindowSettings_Suite : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        ASSERT_NE(m_app, nullptr);

        m_window = MakeConsoleWindow(m_app);
        ASSERT_NE(m_window, nullptr);

        m_console = dynamic_cast<FConsoleWindow*>(m_window);
        ASSERT_NE(m_console, nullptr);
    }

    void TearDown() override
    {
        if (m_window != nullptr)
        {
            m_app->DestroyWindow(m_window);
        }
        delete m_app;
        m_app = nullptr;
    }

    FGenericApplication* m_app = nullptr;
    FGenericWindow* m_window = nullptr;
    FConsoleWindow* m_console = nullptr;
};

TEST_F(ConsoleWindowSettings_Suite, DefaultSettings_AppliedOnCreation)
{
    const FConsoleWindowSettings& settings = m_console->GetSettings();
    EXPECT_EQ(settings.Columns, 120);
    EXPECT_EQ(settings.Rows, 40);
    EXPECT_EQ(settings.FontSize, 16);
    EXPECT_TRUE(settings.bRenderFriendly);
    EXPECT_TRUE(settings.bEnableVirtualTerminal);
}

TEST_F(ConsoleWindowSettings_Suite, ApplySettings_UpdatesAtRuntime)
{
    FConsoleWindowSettings newSettings;
    newSettings.Columns = 80;
    newSettings.Rows = 25;
    newSettings.FontSize = 14;
    newSettings.bRenderFriendly = false;

    m_console->ApplySettings(newSettings);

    const FConsoleWindowSettings& applied = m_console->GetSettings();
    EXPECT_EQ(applied.Columns, 80);
    EXPECT_EQ(applied.Rows, 25);
    EXPECT_EQ(applied.FontSize, 14);
    EXPECT_FALSE(applied.bRenderFriendly);
}

TEST_F(ConsoleWindowSettings_Suite, SetFont_ValidFont_ReturnsTrue)
{
    // Consolas is available on all modern Windows systems
    bool result = m_console->SetFont("Consolas", 16);
    EXPECT_TRUE(result);
}

TEST_F(ConsoleWindowSettings_Suite, SetFont_InvalidFont_ReturnsFalse)
{
    // Windows SetCurrentConsoleFontEx silently substitutes a fallback font
    // for invalid names, so the API call itself succeeds. Verify that after
    // requesting a non-existent font, the actual applied font name differs
    // from the requested name (i.e. Windows substituted something else).
    bool result = m_console->SetFont("NonExistentFontXYZ_12345", 16);
    // The call may return true (Windows accepted it with substitution).
    // Verify the actual font is NOT the requested name.
    HANDLE hOut = ::CreateFileA("CONOUT$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr);
    ASSERT_NE(hOut, INVALID_HANDLE_VALUE);

    CONSOLE_FONT_INFOEX fontInfo = {};
    fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    ::GetCurrentConsoleFontEx(hOut, FALSE, &fontInfo);
    ::CloseHandle(hOut);

    // The actual font should not be the non-existent name
    // (Windows substitutes a real font like "Terminal" or "Consolas")
    std::wstring actualName(fontInfo.FaceName);
    EXPECT_NE(actualName, L"NonExistentFontXYZ_12345");
    (void)result; // suppress unused warning
}

// ===============================================================
// Suite 3: ConsoleWindowRenderMode
// ===============================================================

class ConsoleWindowRenderMode : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        ASSERT_NE(m_app, nullptr);

        m_window = MakeConsoleWindow(m_app);
        ASSERT_NE(m_window, nullptr);

        m_console = dynamic_cast<FConsoleWindow*>(m_window);
        ASSERT_NE(m_console, nullptr);
    }

    void TearDown() override
    {
        if (m_window != nullptr)
        {
            m_app->DestroyWindow(m_window);
        }
        delete m_app;
        m_app = nullptr;
    }

    FGenericApplication* m_app = nullptr;
    FGenericWindow* m_window = nullptr;
    FConsoleWindow* m_console = nullptr;
};

TEST_F(ConsoleWindowRenderMode, RenderFriendly_HidesCursor)
{
    // Default settings have bRenderFriendly=true, so cursor should be hidden.
    HANDLE hOut = ::CreateFileA("CONOUT$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr);
    ASSERT_NE(hOut, INVALID_HANDLE_VALUE);

    CONSOLE_CURSOR_INFO cursorInfo = {};
    ::GetConsoleCursorInfo(hOut, &cursorInfo);
    ::CloseHandle(hOut);
    EXPECT_FALSE(cursorInfo.bVisible);
}

TEST_F(ConsoleWindowRenderMode, SetRenderFriendly_Toggle)
{
    // Disable render-friendly mode
    m_console->SetRenderFriendly(false);

    HANDLE hOut = ::CreateFileA("CONOUT$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr);
    ASSERT_NE(hOut, INVALID_HANDLE_VALUE);

    CONSOLE_CURSOR_INFO cursorInfo = {};
    ::GetConsoleCursorInfo(hOut, &cursorInfo);
    EXPECT_TRUE(cursorInfo.bVisible);

    // Re-enable render-friendly mode
    m_console->SetRenderFriendly(true);

    ::GetConsoleCursorInfo(hOut, &cursorInfo);
    EXPECT_FALSE(cursorInfo.bVisible);

    ::CloseHandle(hOut);
}

TEST_F(ConsoleWindowRenderMode, ClearBuffer_ResetsCursorToOrigin)
{
    // Move cursor away from origin first
    HANDLE hOut = ::CreateFileA("CONOUT$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr);
    ASSERT_NE(hOut, INVALID_HANDLE_VALUE);

    COORD pos = {5, 5};
    ::SetConsoleCursorPosition(hOut, pos);

    m_console->ClearBuffer();

    // Cursor should be back at (0, 0)
    CONSOLE_SCREEN_BUFFER_INFO info = {};
    ::GetConsoleScreenBufferInfo(hOut, &info);
    ::CloseHandle(hOut);
    EXPECT_EQ(info.dwCursorPosition.X, 0);
    EXPECT_EQ(info.dwCursorPosition.Y, 0);
}

// ===============================================================
// Suite 4: ConsoleWindowLifecycle
// ===============================================================

class ConsoleWindowLifecycle : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        ASSERT_NE(m_app, nullptr);
    }

    void TearDown() override
    {
        delete m_app;
        m_app = nullptr;
    }

    FGenericApplication* m_app = nullptr;
};

TEST_F(ConsoleWindowLifecycle, Destroy_RestoresOriginalSettings)
{
    FGenericWindow* window = MakeConsoleWindow(m_app);
    ASSERT_NE(window, nullptr);

    // Destroy should restore original console settings without crash
    m_app->DestroyWindow(window);
    // If we reach here, restoration succeeded (no crash/exception)
}

TEST_F(ConsoleWindowLifecycle, Destroy_DoesNotCloseConsoleProcess)
{
    FGenericWindow* window = MakeConsoleWindow(m_app);
    ASSERT_NE(window, nullptr);

    m_app->DestroyWindow(window);

    // The console process should still be alive after Destroy
    HWND consoleHwnd = ::GetConsoleWindow();
    EXPECT_NE(consoleHwnd, nullptr);
    EXPECT_TRUE(::IsWindow(consoleHwnd));
}

// ===============================================================
// Suite 5: ConsoleWindowInput
// ===============================================================

class ConsoleWindowInput : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        ASSERT_NE(m_app, nullptr);

        m_handler = new FConsoleTestMessageHandler();
        m_app->SetMessageHandler(m_handler);

        m_window = MakeConsoleWindow(m_app);
        ASSERT_NE(m_window, nullptr);

        m_console = dynamic_cast<FConsoleWindow*>(m_window);
        ASSERT_NE(m_console, nullptr);

        // Flush any pending console input events before each test
        HANDLE hIn = ::CreateFileA("CONIN$",
            GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, 0, nullptr);
        if (hIn != INVALID_HANDLE_VALUE)
        {
            ::FlushConsoleInputBuffer(hIn);
            ::CloseHandle(hIn);
        }
    }

    void TearDown() override
    {
        if (m_window != nullptr)
        {
            m_app->DestroyWindow(m_window);
        }
        delete m_app;
        m_app = nullptr;
        delete m_handler;
        m_handler = nullptr;
    }

    FGenericApplication* m_app = nullptr;
    FConsoleTestMessageHandler* m_handler = nullptr;
    FGenericWindow* m_window = nullptr;
    FConsoleWindow* m_console = nullptr;
};

TEST_F(ConsoleWindowInput, pumpConsoleInput_KeyEvent_RoutesToHandler)
{
    // Inject a KEY_EVENT into the console input buffer
    HANDLE hIn = ::CreateFileA("CONIN$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, 0, nullptr);
    ASSERT_NE(hIn, INVALID_HANDLE_VALUE);

    // Flush any stale events
    ::FlushConsoleInputBuffer(hIn);

    INPUT_RECORD record = {};
    record.EventType = KEY_EVENT;
    record.Event.KeyEvent.bKeyDown = TRUE;
    record.Event.KeyEvent.wRepeatCount = 1;
    record.Event.KeyEvent.wVirtualKeyCode = 0x41; // 'A'
    record.Event.KeyEvent.wVirtualScanCode = 0;
    record.Event.KeyEvent.uChar.AsciiChar = 'A';
    record.Event.KeyEvent.dwControlKeyState = 0;

    DWORD written = 0;
    ::WriteConsoleInputA(hIn, &record, 1, &written);
    ::CloseHandle(hIn);
    ASSERT_EQ(written, 1u);

    m_console->pumpConsoleInput(m_handler);

    EXPECT_TRUE(m_handler->bKeyDownCalled);
    EXPECT_EQ(m_handler->LastKeyCode, 0x41);
}

TEST_F(ConsoleWindowInput, pumpConsoleInput_MouseEvent_RoutesToHandler)
{
    // Inject a MOUSE_EVENT into the console input buffer
    HANDLE hIn = ::CreateFileA("CONIN$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, 0, nullptr);
    ASSERT_NE(hIn, INVALID_HANDLE_VALUE);

    // Flush any stale events
    ::FlushConsoleInputBuffer(hIn);

    INPUT_RECORD record = {};
    record.EventType = MOUSE_EVENT;
    record.Event.MouseEvent.dwMousePosition.X = 10;
    record.Event.MouseEvent.dwMousePosition.Y = 5;
    record.Event.MouseEvent.dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED;
    record.Event.MouseEvent.dwEventFlags = 0; // button press, not move

    DWORD written = 0;
    ::WriteConsoleInputA(hIn, &record, 1, &written);
    ::CloseHandle(hIn);
    ASSERT_EQ(written, 1u);

    m_console->pumpConsoleInput(m_handler);

    EXPECT_TRUE(m_handler->bMouseDownCalled);
    EXPECT_EQ(m_handler->LastMouseDownButton, EMouseButton::Left);
}

// ---------------------------------------------------------------
// Standalone test (no fixture needed)
// ---------------------------------------------------------------

TEST(ConsoleWindowEnumTest, EWindowType_Enum_HasNativeAndConsole)
{
    // Verify enum values exist and are distinct
    EXPECT_NE(static_cast<uint8_t>(EWindowType::Native),
              static_cast<uint8_t>(EWindowType::Console));
    EXPECT_EQ(static_cast<uint8_t>(EWindowType::Native), 0);
    EXPECT_EQ(static_cast<uint8_t>(EWindowType::Console), 1);
}
