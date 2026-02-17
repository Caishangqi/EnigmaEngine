// Copyright EnigmaEngine. All Rights Reserved.
// ApplicationCore.Tests -- Window lifecycle tests.

#include "GenericPlatform/GenericApplication.h"
#include "GenericPlatform/GenericWindow.h"
#include "GenericPlatform/GenericWindowDefinition.h"

#include <gtest/gtest.h>

using namespace Enigma;

// Test fixture: creates and destroys the platform application.
class ApplicationCore_Window : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        ASSERT_NE(m_app, nullptr);
    }

    void TearDown() override
    {
        if (m_app != nullptr)
        {
            delete m_app;
            m_app = nullptr;
        }
    }

    FGenericApplication* m_app = nullptr;
};

TEST_F(ApplicationCore_Window, MakeWindowReturnsValidPointer)
{
    FWindowDefinition def;
    def.Title = "Test Window";
    def.bShowOnCreation = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    ASSERT_NE(window, nullptr);

    m_app->DestroyWindow(window);
}

TEST_F(ApplicationCore_Window, MakeWindowReturnsValidNativeHandle)
{
    FWindowDefinition def;
    def.Title = "Test Native Handle";
    def.bShowOnCreation = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    ASSERT_NE(window, nullptr);
    EXPECT_NE(window->GetNativeHandle(), nullptr);

    m_app->DestroyWindow(window);
}

TEST_F(ApplicationCore_Window, GetWidthHeightMatchDefinition)
{
    FWindowDefinition def;
    def.Title = "Test Size";
    def.Width = 800;
    def.Height = 600;
    def.bShowOnCreation = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    ASSERT_NE(window, nullptr);

    EXPECT_EQ(window->GetWidth(), 800);
    EXPECT_EQ(window->GetHeight(), 600);

    m_app->DestroyWindow(window);
}

TEST_F(ApplicationCore_Window, ShowHideToggleVisibility)
{
    FWindowDefinition def;
    def.Title = "Test Visibility";
    def.bShowOnCreation = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    ASSERT_NE(window, nullptr);

    // Initially hidden (bShowOnCreation = false)
    EXPECT_FALSE(window->IsVisible());

    window->Show();
    EXPECT_TRUE(window->IsVisible());

    window->Hide();
    EXPECT_FALSE(window->IsVisible());

    m_app->DestroyWindow(window);
}

TEST_F(ApplicationCore_Window, DestroyWindowReleasesResources)
{
    FWindowDefinition def;
    def.Title = "Test Destroy";
    def.bShowOnCreation = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    ASSERT_NE(window, nullptr);

    // DestroyWindow should not crash
    m_app->DestroyWindow(window);
    // window pointer is now dangling -- do not use
}

TEST_F(ApplicationCore_Window, MultipleWindowsCanBeCreated)
{
    FWindowDefinition def1;
    def1.Title = "Window 1";
    def1.bShowOnCreation = false;

    FWindowDefinition def2;
    def2.Title = "Window 2";
    def2.bShowOnCreation = false;

    FGenericWindow* w1 = m_app->MakeWindow(def1);
    FGenericWindow* w2 = m_app->MakeWindow(def2);

    ASSERT_NE(w1, nullptr);
    ASSERT_NE(w2, nullptr);
    EXPECT_NE(w1, w2);
    EXPECT_NE(w1->GetNativeHandle(), w2->GetNativeHandle());

    m_app->DestroyWindow(w2);
    m_app->DestroyWindow(w1);
}

TEST_F(ApplicationCore_Window, ResizeUpdatesWidthHeight)
{
    FWindowDefinition def;
    def.Title = "Test Resize";
    def.Width = 640;
    def.Height = 480;
    def.bShowOnCreation = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    ASSERT_NE(window, nullptr);

    window->Resize(1024, 768);
    EXPECT_EQ(window->GetWidth(), 1024);
    EXPECT_EQ(window->GetHeight(), 768);

    m_app->DestroyWindow(window);
}

TEST_F(ApplicationCore_Window, SetTitleDoesNotCrash)
{
    FWindowDefinition def;
    def.Title = "Original Title";
    def.bShowOnCreation = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    ASSERT_NE(window, nullptr);

    // SetTitle should not crash
    window->SetTitle("New Title");

    m_app->DestroyWindow(window);
}
