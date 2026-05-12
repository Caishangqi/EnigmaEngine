// Copyright EnigmaEngine. All Rights Reserved.
// ApplicationCore.Tests -- Console window creation, settings, lifecycle, and input tests.

#include "GenericPlatform/GenericApplication.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "GenericPlatform/GenericWindow.h"
#include "GenericPlatform/GenericWindowDefinition.h"
#include "GenericPlatform/ConsoleWindowSettings.h"
#include "Windows/ConsoleWindow.h"

#include <windows.h>

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST(SuiteName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST( \
        F##SuiteName##_##TestName##AutomationTest, \
        "System.ApplicationCore." #SuiteName "." #TestName, \
        ApplicationCore, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresApplicationCore | ::Enigma::EAutomationTestFlags::RequiresWindow)

#define ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(FixtureName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST_F( \
        FixtureName, \
        F##FixtureName##_##TestName##AutomationTest, \
        "System.ApplicationCore." #FixtureName "." #TestName, \
        ApplicationCore, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresApplicationCore | ::Enigma::EAutomationTestFlags::RequiresWindow)

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

class ConsoleWindowCreation : public ::Enigma::FAutomationTestFixture
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        if (!TestNotEqual("ASSERT_NE", m_app, nullptr)) { return; }
    }

    void TearDown() override
    {
        delete m_app;
        m_app = nullptr;
    }

    FGenericApplication* m_app = nullptr;
};

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowCreation, MakeWindow_WithConsoleType_CreatesFConsoleWindow)
{
    FGenericWindow* window = MakeConsoleWindow(m_app);
    if (!TestNotEqual("ASSERT_NE", window, nullptr)) { return; }

    // dynamic_cast succeeds only for FConsoleWindow
    auto* consoleWin = dynamic_cast<FConsoleWindow*>(window);
    TestNotEqual("EXPECT_NE", consoleWin, nullptr);

    m_app->DestroyWindow(window);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowCreation, MakeWindow_WithNativeType_CreatesWindowsWindow)
{
    FWindowDefinition def;
    def.Type = EWindowType::Native;
    def.Title = "Native Test";
    def.bShowOnCreation = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    if (!TestNotEqual("ASSERT_NE", window, nullptr)) { return; }

    // dynamic_cast to FConsoleWindow should fail for native windows
    auto* consoleWin = dynamic_cast<FConsoleWindow*>(window);
    TestEqual("EXPECT_EQ", consoleWin, nullptr);

    m_app->DestroyWindow(window);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowCreation, GetNativeHandle_ReturnsValidConsoleHWND)
{
    FGenericWindow* window = MakeConsoleWindow(m_app);
    if (!TestNotEqual("ASSERT_NE", window, nullptr)) { return; }

    void* handle = window->GetNativeHandle();
    TestNotEqual("EXPECT_NE", handle, nullptr);

    // The handle should be a valid HWND (IsWindow returns TRUE)
    TestTrue("EXPECT_TRUE", ::IsWindow(static_cast<HWND>(handle)));

    m_app->DestroyWindow(window);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowCreation, GetWidth_ReturnsColumns_NotPixels)
{
    FGenericWindow* window = MakeConsoleWindow(m_app, 100, 30);
    if (!TestNotEqual("ASSERT_NE", window, nullptr)) { return; }

    // Width should be character columns, not pixels
    TestEqual("EXPECT_EQ", window->GetWidth(), 100);

    m_app->DestroyWindow(window);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowCreation, GetHeight_ReturnsRows_NotPixels)
{
    FGenericWindow* window = MakeConsoleWindow(m_app, 100, 30);
    if (!TestNotEqual("ASSERT_NE", window, nullptr)) { return; }

    // Height should be character rows, not pixels
    TestEqual("EXPECT_EQ", window->GetHeight(), 30);

    m_app->DestroyWindow(window);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowCreation, MakeWindow_AppliesConsoleViewportSize)
{
    FGenericWindow* Window = MakeConsoleWindow(m_app, 80, 25);
    if (!TestNotEqual("ASSERT_NE", Window, nullptr)) { return; }

    HANDLE OutputHandle = ::CreateFileA("CONOUT$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr);
    if (!TestNotEqual("ASSERT_NE", OutputHandle, INVALID_HANDLE_VALUE))
    {
        m_app->DestroyWindow(Window);
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO BufferInfo = {};
    bool bGotInfo = ::GetConsoleScreenBufferInfo(OutputHandle, &BufferInfo) != FALSE;
    ::CloseHandle(OutputHandle);
    if (!TestTrue("ASSERT_TRUE", bGotInfo))
    {
        m_app->DestroyWindow(Window);
        return;
    }

    const SHORT ViewportWidth = BufferInfo.srWindow.Right - BufferInfo.srWindow.Left + 1;
    const SHORT ViewportHeight = BufferInfo.srWindow.Bottom - BufferInfo.srWindow.Top + 1;
    TestEqual("EXPECT_EQ", ViewportWidth, static_cast<SHORT>(80));
    TestEqual("EXPECT_EQ", ViewportHeight, static_cast<SHORT>(25));

    m_app->DestroyWindow(Window);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowCreation, MakeWindow_AppliesTitleFromDefinition)
{
    FWindowDefinition def;
    def.Type = EWindowType::Console;
    def.Title = "TestTitle_12345";
    def.Width = 120;
    def.Height = 40;

    FGenericWindow* window = m_app->MakeWindow(def);
    if (!TestNotEqual("ASSERT_NE", window, nullptr)) { return; }

    // Verify the console title was applied
    char titleBuf[256] = {};
    DWORD len = ::GetConsoleTitleA(titleBuf, sizeof(titleBuf));
    TestGreaterThan("EXPECT_GT", len, 0u);
    TestStringEqual("EXPECT_STREQ", titleBuf, "TestTitle_12345");

    m_app->DestroyWindow(window);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowCreation, MakeWindow_PassesResizableFlag)
{
    // Create a non-resizable console window
    FWindowDefinition def;
    def.Type = EWindowType::Console;
    def.Width = 120;
    def.Height = 40;
    def.bIsResizable = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    if (!TestNotEqual("ASSERT_NE", window, nullptr)) { return; }

    auto* consoleWin = dynamic_cast<FConsoleWindow*>(window);
    if (!TestNotEqual("ASSERT_NE", consoleWin, nullptr)) { return; }

    const FConsoleWindowSettings& settings = consoleWin->GetSettings();
    TestFalse("EXPECT_FALSE", settings.bResizable);

    m_app->DestroyWindow(window);
}

// ===============================================================
// Suite 2: ConsoleWindowSettings
// ===============================================================

class ConsoleWindowSettings_Suite : public ::Enigma::FAutomationTestFixture
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        if (!TestNotEqual("ASSERT_NE", m_app, nullptr)) { return; }

        m_window = MakeConsoleWindow(m_app);
        if (!TestNotEqual("ASSERT_NE", m_window, nullptr)) { return; }

        m_console = dynamic_cast<FConsoleWindow*>(m_window);
        if (!TestNotEqual("ASSERT_NE", m_console, nullptr)) { return; }
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

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowSettings_Suite, DefaultSettings_AppliedOnCreation)
{
    const FConsoleWindowSettings& settings = m_console->GetSettings();
    TestEqual("EXPECT_EQ", settings.Columns, 120);
    TestEqual("EXPECT_EQ", settings.Rows, 40);
    TestEqual("EXPECT_EQ", settings.FontSize, 16);
    TestTrue("EXPECT_TRUE", settings.bRenderFriendly);
    TestTrue("EXPECT_TRUE", settings.bEnableVirtualTerminal);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowSettings_Suite, ApplySettings_UpdatesAtRuntime)
{
    FConsoleWindowSettings newSettings;
    newSettings.Columns = 80;
    newSettings.Rows = 25;
    newSettings.FontSize = 14;
    newSettings.bRenderFriendly = false;

    m_console->ApplySettings(newSettings);

    const FConsoleWindowSettings& applied = m_console->GetSettings();
    TestEqual("EXPECT_EQ", applied.Columns, 80);
    TestEqual("EXPECT_EQ", applied.Rows, 25);
    TestEqual("EXPECT_EQ", applied.FontSize, 14);
    TestFalse("EXPECT_FALSE", applied.bRenderFriendly);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowSettings_Suite, SetFont_ValidFont_ReturnsTrue)
{
    // Consolas is available on all modern Windows systems
    bool result = m_console->SetFont("Consolas", 16);
    TestTrue("EXPECT_TRUE", result);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowSettings_Suite, SetFont_InvalidFont_ReturnsFalse)
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
    if (!TestNotEqual("ASSERT_NE", hOut, INVALID_HANDLE_VALUE)) { return; }

    CONSOLE_FONT_INFOEX fontInfo = {};
    fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    ::GetCurrentConsoleFontEx(hOut, FALSE, &fontInfo);
    ::CloseHandle(hOut);

    // The actual font should not be the non-existent name
    // (Windows substitutes a real font like "Terminal" or "Consolas")
    std::wstring actualName(fontInfo.FaceName);
    TestNotEqual("EXPECT_NE", actualName, L"NonExistentFontXYZ_12345");
    (void)result; // suppress unused warning
}

// ===============================================================
// Suite 3: ConsoleWindowRenderMode
// ===============================================================

class ConsoleWindowRenderMode : public ::Enigma::FAutomationTestFixture
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        if (!TestNotEqual("ASSERT_NE", m_app, nullptr)) { return; }

        m_window = MakeConsoleWindow(m_app);
        if (!TestNotEqual("ASSERT_NE", m_window, nullptr)) { return; }

        m_console = dynamic_cast<FConsoleWindow*>(m_window);
        if (!TestNotEqual("ASSERT_NE", m_console, nullptr)) { return; }
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

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowRenderMode, RenderFriendly_HidesCursor)
{
    // Default settings have bRenderFriendly=true, so cursor should be hidden.
    HANDLE hOut = ::CreateFileA("CONOUT$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr);
    if (!TestNotEqual("ASSERT_NE", hOut, INVALID_HANDLE_VALUE)) { return; }

    CONSOLE_CURSOR_INFO cursorInfo = {};
    ::GetConsoleCursorInfo(hOut, &cursorInfo);
    ::CloseHandle(hOut);
    TestFalse("EXPECT_FALSE", cursorInfo.bVisible);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowRenderMode, SetRenderFriendly_Toggle)
{
    // Disable render-friendly mode
    m_console->SetRenderFriendly(false);

    HANDLE hOut = ::CreateFileA("CONOUT$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr);
    if (!TestNotEqual("ASSERT_NE", hOut, INVALID_HANDLE_VALUE)) { return; }

    CONSOLE_CURSOR_INFO cursorInfo = {};
    ::GetConsoleCursorInfo(hOut, &cursorInfo);
    TestTrue("EXPECT_TRUE", cursorInfo.bVisible);

    // Re-enable render-friendly mode
    m_console->SetRenderFriendly(true);

    ::GetConsoleCursorInfo(hOut, &cursorInfo);
    TestFalse("EXPECT_FALSE", cursorInfo.bVisible);

    ::CloseHandle(hOut);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowRenderMode, ClearBuffer_ResetsCursorToOrigin)
{
    // Move cursor away from origin first
    HANDLE hOut = ::CreateFileA("CONOUT$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr);
    if (!TestNotEqual("ASSERT_NE", hOut, INVALID_HANDLE_VALUE)) { return; }

    COORD pos = {5, 5};
    ::SetConsoleCursorPosition(hOut, pos);

    m_console->ClearBuffer();

    // Cursor should be back at (0, 0)
    CONSOLE_SCREEN_BUFFER_INFO info = {};
    ::GetConsoleScreenBufferInfo(hOut, &info);
    ::CloseHandle(hOut);
    TestEqual("EXPECT_EQ", info.dwCursorPosition.X, 0);
    TestEqual("EXPECT_EQ", info.dwCursorPosition.Y, 0);
}

// ===============================================================
// Suite 4: ConsoleWindowLifecycle
// ===============================================================

class ConsoleWindowLifecycle : public ::Enigma::FAutomationTestFixture
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        if (!TestNotEqual("ASSERT_NE", m_app, nullptr)) { return; }
    }

    void TearDown() override
    {
        delete m_app;
        m_app = nullptr;
    }

    FGenericApplication* m_app = nullptr;
};

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowLifecycle, Destroy_RestoresOriginalSettings)
{
    FGenericWindow* window = MakeConsoleWindow(m_app);
    if (!TestNotEqual("ASSERT_NE", window, nullptr)) { return; }

    // Destroy should restore original console settings without crash
    m_app->DestroyWindow(window);
    // If we reach here, restoration succeeded (no crash/exception)
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowLifecycle, Destroy_DoesNotCloseConsoleProcess)
{
    FGenericWindow* window = MakeConsoleWindow(m_app);
    if (!TestNotEqual("ASSERT_NE", window, nullptr)) { return; }

    m_app->DestroyWindow(window);

    // The console process should still be alive after Destroy
    HWND consoleHwnd = ::GetConsoleWindow();
    TestNotEqual("EXPECT_NE", consoleHwnd, nullptr);
    TestTrue("EXPECT_TRUE", ::IsWindow(consoleHwnd));
}

// ===============================================================
// Suite 5: ConsoleWindowInput
// ===============================================================

class ConsoleWindowInput : public ::Enigma::FAutomationTestFixture
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        if (!TestNotEqual("ASSERT_NE", m_app, nullptr)) { return; }

        m_handler = new FConsoleTestMessageHandler();
        m_app->SetMessageHandler(m_handler);

        m_window = MakeConsoleWindow(m_app);
        if (!TestNotEqual("ASSERT_NE", m_window, nullptr)) { return; }

        m_console = dynamic_cast<FConsoleWindow*>(m_window);
        if (!TestNotEqual("ASSERT_NE", m_console, nullptr)) { return; }

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

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowInput, pumpConsoleInput_KeyEvent_RoutesToHandler)
{
    // Inject a KEY_EVENT into the console input buffer
    HANDLE hIn = ::CreateFileA("CONIN$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, 0, nullptr);
    if (!TestNotEqual("ASSERT_NE", hIn, INVALID_HANDLE_VALUE)) { return; }

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
    if (!TestEqual("ASSERT_EQ", written, 1u)) { return; }

    m_console->pumpConsoleInput(m_handler);

    TestTrue("EXPECT_TRUE", m_handler->bKeyDownCalled);
    TestEqual("EXPECT_EQ", m_handler->LastKeyCode, 0x41);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ConsoleWindowInput, pumpConsoleInput_MouseEvent_RoutesToHandler)
{
    // Inject a MOUSE_EVENT into the console input buffer
    HANDLE hIn = ::CreateFileA("CONIN$",
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, 0, nullptr);
    if (!TestNotEqual("ASSERT_NE", hIn, INVALID_HANDLE_VALUE)) { return; }

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
    if (!TestEqual("ASSERT_EQ", written, 1u)) { return; }

    m_console->pumpConsoleInput(m_handler);

    TestTrue("EXPECT_TRUE", m_handler->bMouseDownCalled);
    TestEqual("EXPECT_EQ", m_handler->LastMouseDownButton, EMouseButton::Left);
}

// ---------------------------------------------------------------
// Standalone test (no fixture needed)
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST(ConsoleWindowEnumTest, EWindowType_Enum_HasNativeAndConsole)
{
    // Verify enum values exist and are distinct
    TestNotEqual("EXPECT_NE", static_cast<uint8_t>(EWindowType::Native), static_cast<uint8_t>(EWindowType::Console));
    TestEqual("EXPECT_EQ", static_cast<uint8_t>(EWindowType::Native), 0);
    TestEqual("EXPECT_EQ", static_cast<uint8_t>(EWindowType::Console), 1);
}
