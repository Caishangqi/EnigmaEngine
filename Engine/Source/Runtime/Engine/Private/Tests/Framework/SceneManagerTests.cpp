// Copyright EnigmaEngine. All Rights Reserved.

/// @file SceneManagerTests.cpp
/// @brief Unit tests for FSceneManager scene transitions.

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SuiteName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST( \
        F##SuiteName##_##TestName##AutomationTest, \
        "System.Engine.Framework." #SuiteName "." #TestName, \
        Engine, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresEngine)

#define ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(FixtureName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST_F( \
        FixtureName, \
        F##FixtureName##_##TestName##AutomationTest, \
        "System.Engine.Framework." #FixtureName "." #TestName, \
        Engine, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresEngine)

#include "GameFramework/SceneManager.h"

using namespace Enigma;

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneManagerTest, GetActiveScene_NullWhenEmpty)
{
	FSceneManager mgr;
	TestEqual("EXPECT_EQ", mgr.GetActiveScene(), nullptr);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneManagerTest, GetActiveSceneName_EmptyWhenNoScene)
{
	FSceneManager mgr;
	TestTrue("EXPECT_TRUE", mgr.GetActiveSceneName().empty());
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneManagerTest, LoadScene_ReturnsPendingScene)
{
	FSceneManager mgr;
	FScene* pending = mgr.LoadScene("Level1");
	TestNotEqual("EXPECT_NE", pending, nullptr);
	// Not yet active -- transition happens at next Tick
	TestEqual("EXPECT_EQ", mgr.GetActiveScene(), nullptr);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneManagerTest, Tick_TransitionsPendingToActive)
{
	FSceneManager mgr;
	mgr.LoadScene("Level1");
	TestEqual("EXPECT_EQ", mgr.GetActiveScene(), nullptr);

	mgr.Tick(0.016f);
	TestNotEqual("EXPECT_NE", mgr.GetActiveScene(), nullptr);
	TestEqual("EXPECT_EQ", mgr.GetActiveSceneName(), "Level1");
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneManagerTest, Tick_DestroysOldScene)
{
	FSceneManager mgr;
	mgr.LoadScene("Level1");
	mgr.Tick(0.016f);
	FScene* oldScene = mgr.GetActiveScene();
	if (!TestNotEqual("ASSERT_NE", oldScene, nullptr)) { return; }

	mgr.LoadScene("Level2");
	mgr.Tick(0.016f);
	// Old scene should be destroyed, new scene active
	TestNotEqual("EXPECT_NE", mgr.GetActiveScene(), oldScene);
	TestEqual("EXPECT_EQ", mgr.GetActiveSceneName(), "Level2");
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneManagerTest, LoadScene_MidTick_DeferredToNextTick)
{
	FSceneManager mgr;
	mgr.LoadScene("Level1");
	mgr.Tick(0.016f);
	TestEqual("EXPECT_EQ", mgr.GetActiveSceneName(), "Level1");

	// Load a new scene -- should not take effect until next Tick
	mgr.LoadScene("Level2");
	TestEqual("EXPECT_EQ", mgr.GetActiveSceneName(), "Level1"); // still Level1

	mgr.Tick(0.016f);
	TestEqual("EXPECT_EQ", mgr.GetActiveSceneName(), "Level2"); // now Level2
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneManagerTest, RenderScene_DelegatesToActiveScene)
{
	FSceneManager mgr;
	// No active scene -- should not crash
	mgr.RenderScene();

	mgr.LoadScene("Level1");
	mgr.Tick(0.016f);
	// Active scene exists -- should not crash
	mgr.RenderScene();
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneManagerTest, MultipleScenesInSequence)
{
	FSceneManager mgr;

	mgr.LoadScene("A");
	mgr.Tick(0.0f);
	TestEqual("EXPECT_EQ", mgr.GetActiveSceneName(), "A");

	mgr.LoadScene("B");
	mgr.Tick(0.0f);
	TestEqual("EXPECT_EQ", mgr.GetActiveSceneName(), "B");

	mgr.LoadScene("C");
	mgr.Tick(0.0f);
	TestEqual("EXPECT_EQ", mgr.GetActiveSceneName(), "C");
}
