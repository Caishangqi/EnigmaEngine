// Copyright EnigmaEngine. All Rights Reserved.
// ApplicationCore.Tests -- Message handler routing tests.

#include "GenericPlatform/GenericApplication.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "GenericPlatform/GenericWindow.h"
#include "GenericPlatform/GenericWindowDefinition.h"

#include <windows.h>

#include <gtest/gtest.h>

using namespace Enigma;

// A test message handler that records which callbacks were invoked.
class FTestMessageHandler : public FGenericApplicationMessageHandler
{
public:
    bool bKeyDownCalled = false;
    int32_t LastKeyCode = 0;

    bool bKeyUpCalled = false;
    bool bKeyCharCalled = false;
    uint32_t LastCharacter = 0;

    bool bMouseDownCalled = false;
    EMouseButton LastMouseButton = EMouseButton::Left;

    bool bMouseUpCalled = false;
    bool bMouseMoveCalled = false;
    float LastCursorX = 0.0f;
    float LastCursorY = 0.0f;

    bool bWindowResizedCalled = false;
    int32_t LastResizeWidth = 0;
    int32_t LastResizeHeight = 0;

    bool bWindowFocusCalled = false;
    bool LastFocusState = false;

    bool bWindowCloseRequestedCalled = false;
    bool CloseRequestResult = true;

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
        LastMouseButton = button;
        LastCursorX = cursorX;
        LastCursorY = cursorY;
        return true;
    }

    bool OnMouseUp(EMouseButton button, float cursorX, float cursorY) override
    {
        bMouseUpCalled = true;
        LastMouseButton = button;
        LastCursorX = cursorX;
        LastCursorY = cursorY;
        return true;
    }

    bool OnMouseMove(float cursorX, float cursorY) override
    {
        bMouseMoveCalled = true;
        LastCursorX = cursorX;
        LastCursorY = cursorY;
        return true;
    }

    void OnWindowResized(FGenericWindow* /*window*/, int32_t width, int32_t height) override
    {
        bWindowResizedCalled = true;
        LastResizeWidth = width;
        LastResizeHeight = height;
    }

    void OnWindowFocusChanged(FGenericWindow* /*window*/, bool bHasFocus) override
    {
        bWindowFocusCalled = true;
        LastFocusState = bHasFocus;
    }

    bool OnWindowCloseRequested(FGenericWindow* /*window*/) override
    {
        bWindowCloseRequestedCalled = true;
        return CloseRequestResult;
    }
};

// Test fixture
class ApplicationCore_MessageHandler : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        ASSERT_NE(m_app, nullptr);

        m_handler = new FTestMessageHandler();
        m_app->SetMessageHandler(m_handler);

        FWindowDefinition def;
        def.Title = "Handler Test";
        def.Width = 640;
        def.Height = 480;
        def.bShowOnCreation = false;
        m_window = m_app->MakeWindow(def);
        ASSERT_NE(m_window, nullptr);

        m_hwnd = static_cast<HWND>(m_window->GetNativeHandle());
        ASSERT_NE(m_hwnd, nullptr);
    }

    void TearDown() override
    {
        if (m_app != nullptr)
        {
            delete m_app;
            m_app = nullptr;
        }
        delete m_handler;
        m_handler = nullptr;
        m_window = nullptr;
    }

    FGenericApplication* m_app = nullptr;
    FTestMessageHandler* m_handler = nullptr;
    FGenericWindow* m_window = nullptr;
    HWND m_hwnd = nullptr;
};

TEST_F(ApplicationCore_MessageHandler, SetMessageHandlerWorks)
{
    EXPECT_EQ(m_app->GetMessageHandler(), m_handler);
}

TEST_F(ApplicationCore_MessageHandler, OnKeyDownCalledViaSendMessage)
{
    // VK_SPACE = 0x20, lParam bit 30 = 0 (not repeat)
    ::SendMessage(m_hwnd, WM_KEYDOWN, VK_SPACE, 0);

    EXPECT_TRUE(m_handler->bKeyDownCalled);
    EXPECT_EQ(m_handler->LastKeyCode, VK_SPACE);
}

TEST_F(ApplicationCore_MessageHandler, OnKeyUpCalledViaSendMessage)
{
    ::SendMessage(m_hwnd, WM_KEYUP, VK_RETURN, 0);

    EXPECT_TRUE(m_handler->bKeyUpCalled);
    EXPECT_EQ(m_handler->LastKeyCode, VK_RETURN);
}

TEST_F(ApplicationCore_MessageHandler, OnKeyCharCalledViaSendMessage)
{
    ::SendMessage(m_hwnd, WM_CHAR, static_cast<WPARAM>('A'), 0);

    EXPECT_TRUE(m_handler->bKeyCharCalled);
    EXPECT_EQ(m_handler->LastCharacter, static_cast<uint32_t>('A'));
}

TEST_F(ApplicationCore_MessageHandler, OnMouseDownLeftButton)
{
    LPARAM lParam = MAKELPARAM(100, 200);
    ::SendMessage(m_hwnd, WM_LBUTTONDOWN, 0, lParam);

    EXPECT_TRUE(m_handler->bMouseDownCalled);
    EXPECT_EQ(m_handler->LastMouseButton, EMouseButton::Left);
    EXPECT_FLOAT_EQ(m_handler->LastCursorX, 100.0f);
    EXPECT_FLOAT_EQ(m_handler->LastCursorY, 200.0f);
}

TEST_F(ApplicationCore_MessageHandler, OnMouseUpRightButton)
{
    LPARAM lParam = MAKELPARAM(50, 75);
    ::SendMessage(m_hwnd, WM_RBUTTONUP, 0, lParam);

    EXPECT_TRUE(m_handler->bMouseUpCalled);
    EXPECT_EQ(m_handler->LastMouseButton, EMouseButton::Right);
    EXPECT_FLOAT_EQ(m_handler->LastCursorX, 50.0f);
    EXPECT_FLOAT_EQ(m_handler->LastCursorY, 75.0f);
}

TEST_F(ApplicationCore_MessageHandler, OnMouseMoveCalledViaSendMessage)
{
    LPARAM lParam = MAKELPARAM(300, 400);
    ::SendMessage(m_hwnd, WM_MOUSEMOVE, 0, lParam);

    EXPECT_TRUE(m_handler->bMouseMoveCalled);
    EXPECT_FLOAT_EQ(m_handler->LastCursorX, 300.0f);
    EXPECT_FLOAT_EQ(m_handler->LastCursorY, 400.0f);
}

TEST_F(ApplicationCore_MessageHandler, OnWindowCloseRequestedAllowsClose)
{
    m_handler->CloseRequestResult = true;
    ::SendMessage(m_hwnd, WM_CLOSE, 0, 0);

    EXPECT_TRUE(m_handler->bWindowCloseRequestedCalled);
    // Window should have been destroyed (pointer is now dangling)
    // Prevent TearDown from double-destroying
    m_window = nullptr;
}

TEST_F(ApplicationCore_MessageHandler, OnWindowCloseRequestedPreventsClose)
{
    m_handler->CloseRequestResult = false;
    ::SendMessage(m_hwnd, WM_CLOSE, 0, 0);

    EXPECT_TRUE(m_handler->bWindowCloseRequestedCalled);
    // Window should still be valid
    EXPECT_NE(m_window->GetNativeHandle(), nullptr);
}
