// Copyright EnigmaEngine. All Rights Reserved.

/// @file SceneManagerTests.cpp
/// @brief Unit tests for FSceneManager scene transitions.

#include <gtest/gtest.h>

#include "GameFramework/SceneManager.h"

using namespace Enigma;

TEST(SceneManagerTest, GetActiveScene_NullWhenEmpty)
{
	FSceneManager mgr;
	EXPECT_EQ(mgr.GetActiveScene(), nullptr);
}

TEST(SceneManagerTest, GetActiveSceneName_EmptyWhenNoScene)
{
	FSceneManager mgr;
	EXPECT_TRUE(mgr.GetActiveSceneName().empty());
}

TEST(SceneManagerTest, LoadScene_ReturnsPendingScene)
{
	FSceneManager mgr;
	FScene* pending = mgr.LoadScene("Level1");
	EXPECT_NE(pending, nullptr);
	// Not yet active -- transition happens at next Tick
	EXPECT_EQ(mgr.GetActiveScene(), nullptr);
}

TEST(SceneManagerTest, Tick_TransitionsPendingToActive)
{
	FSceneManager mgr;
	mgr.LoadScene("Level1");
	EXPECT_EQ(mgr.GetActiveScene(), nullptr);

	mgr.Tick(0.016f);
	EXPECT_NE(mgr.GetActiveScene(), nullptr);
	EXPECT_EQ(mgr.GetActiveSceneName(), "Level1");
}

TEST(SceneManagerTest, Tick_DestroysOldScene)
{
	FSceneManager mgr;
	mgr.LoadScene("Level1");
	mgr.Tick(0.016f);
	FScene* oldScene = mgr.GetActiveScene();
	ASSERT_NE(oldScene, nullptr);

	mgr.LoadScene("Level2");
	mgr.Tick(0.016f);
	// Old scene should be destroyed, new scene active
	EXPECT_NE(mgr.GetActiveScene(), oldScene);
	EXPECT_EQ(mgr.GetActiveSceneName(), "Level2");
}

TEST(SceneManagerTest, LoadScene_MidTick_DeferredToNextTick)
{
	FSceneManager mgr;
	mgr.LoadScene("Level1");
	mgr.Tick(0.016f);
	EXPECT_EQ(mgr.GetActiveSceneName(), "Level1");

	// Load a new scene -- should not take effect until next Tick
	mgr.LoadScene("Level2");
	EXPECT_EQ(mgr.GetActiveSceneName(), "Level1"); // still Level1

	mgr.Tick(0.016f);
	EXPECT_EQ(mgr.GetActiveSceneName(), "Level2"); // now Level2
}

TEST(SceneManagerTest, RenderScene_DelegatesToActiveScene)
{
	FSceneManager mgr;
	// No active scene -- should not crash
	mgr.RenderScene();

	mgr.LoadScene("Level1");
	mgr.Tick(0.016f);
	// Active scene exists -- should not crash
	mgr.RenderScene();
}

TEST(SceneManagerTest, MultipleScenesInSequence)
{
	FSceneManager mgr;

	mgr.LoadScene("A");
	mgr.Tick(0.0f);
	EXPECT_EQ(mgr.GetActiveSceneName(), "A");

	mgr.LoadScene("B");
	mgr.Tick(0.0f);
	EXPECT_EQ(mgr.GetActiveSceneName(), "B");

	mgr.LoadScene("C");
	mgr.Tick(0.0f);
	EXPECT_EQ(mgr.GetActiveSceneName(), "C");
}
