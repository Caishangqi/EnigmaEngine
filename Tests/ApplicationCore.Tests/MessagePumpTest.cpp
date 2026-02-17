// Copyright EnigmaEngine. All Rights Reserved.
// ApplicationCore.Tests -- Message pump safety tests.

#include "GenericPlatform/GenericApplication.h"
#include "GenericPlatform/GenericWindowDefinition.h"

#include <gtest/gtest.h>

using namespace Enigma;

class ApplicationCore_MessagePump : public ::testing::Test
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

TEST_F(ApplicationCore_MessagePump, PumpMessagesWithNoWindowsDoesNotCrash)
{
    // No windows created -- PumpMessages should be safe (no-op on empty queue)
    m_app->PumpMessages(0.016f);
}

TEST_F(ApplicationCore_MessagePump, PumpMessagesWithWindowDoesNotCrash)
{
    FWindowDefinition def;
    def.Title = "Pump Test";
    def.bShowOnCreation = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    ASSERT_NE(window, nullptr);

    // Pump a few frames
    for (int i = 0; i < 5; ++i)
    {
        m_app->PumpMessages(0.016f);
    }

    m_app->DestroyWindow(window);
}

TEST_F(ApplicationCore_MessagePump, ProcessDeferredEventsDoesNotCrash)
{
    // ProcessDeferredEvents is a no-op by default but should not crash
    m_app->ProcessDeferredEvents(0.016f);
}
