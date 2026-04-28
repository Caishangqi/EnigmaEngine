// Copyright EnigmaEngine. All Rights Reserved.
// ApplicationCore.Tests -- Message pump safety tests.

#include "GenericPlatform/GenericApplication.h"
#include "GenericPlatform/GenericWindowDefinition.h"

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

class ApplicationCore_MessagePump : public ::Enigma::FAutomationTestFixture
{
protected:
    void SetUp() override
    {
        m_app = FGenericApplication::CreateApplication();
        if (!TestNotEqual("ASSERT_NE", m_app, nullptr)) { return; }
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

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ApplicationCore_MessagePump, PumpMessagesWithNoWindowsDoesNotCrash)
{
    // No windows created -- PumpMessages should be safe (no-op on empty queue)
    m_app->PumpMessages(0.016f);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ApplicationCore_MessagePump, PumpMessagesWithWindowDoesNotCrash)
{
    FWindowDefinition def;
    def.Title = "Pump Test";
    def.bShowOnCreation = false;

    FGenericWindow* window = m_app->MakeWindow(def);
    if (!TestNotEqual("ASSERT_NE", window, nullptr)) { return; }

    // Pump a few frames
    for (int i = 0; i < 5; ++i)
    {
        m_app->PumpMessages(0.016f);
    }

    m_app->DestroyWindow(window);
}

ENIGMA_IMPLEMENT_APPLICATION_CORE_AUTOMATION_TEST_F(ApplicationCore_MessagePump, ProcessDeferredEventsDoesNotCrash)
{
    // ProcessDeferredEvents is a no-op by default but should not crash
    m_app->ProcessDeferredEvents(0.016f);
}
