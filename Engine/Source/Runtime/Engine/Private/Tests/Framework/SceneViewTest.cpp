// Copyright EnigmaEngine. All Rights Reserved.

/// @file SceneViewTest.cpp
/// @brief Unit tests for FSceneView and ECameraProjectionMode.

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

#include "SceneView/SceneView.h"
#include "Math/Vector.h"

using namespace Enigma;

// ---------------------------------------------------------------
// ECameraProjectionMode
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(CameraProjectionModeTest, EnumValues)
{
	// Verify enum values exist and are distinct.
	auto ortho = ECameraProjectionMode::Orthographic;
	auto persp = ECameraProjectionMode::Perspective;
	TestNotEqual("EXPECT_NE", static_cast<uint8_t>(ortho), static_cast<uint8_t>(persp));
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(CameraProjectionModeTest, Orthographic_IsZero)
{
	TestEqual("EXPECT_EQ", static_cast<uint8_t>(ECameraProjectionMode::Orthographic), 0);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(CameraProjectionModeTest, Perspective_IsOne)
{
	TestEqual("EXPECT_EQ", static_cast<uint8_t>(ECameraProjectionMode::Perspective), 1);
}

// ---------------------------------------------------------------
// FSceneView ??Default construction
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneViewTest, DefaultConstruction_Orthographic)
{
	FSceneView view;
	TestEqual("EXPECT_EQ", view.ProjectionMode, ECameraProjectionMode::Orthographic);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneViewTest, DefaultConstruction_IdentityTransform)
{
	FSceneView view;
	const auto& translation = view.ViewTransform.GetTranslation();
	TestNear("EXPECT_FLOAT_EQ", translation.X, 0.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", translation.Y, 0.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", translation.Z, 0.0f, 1e-6f);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneViewTest, DefaultConstruction_ZeroViewport)
{
	FSceneView view;
	TestEqual("EXPECT_EQ", view.ViewportX, 0);
	TestEqual("EXPECT_EQ", view.ViewportY, 0);
	TestEqual("EXPECT_EQ", view.ViewportWidth, 0);
	TestEqual("EXPECT_EQ", view.ViewportHeight, 0);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneViewTest, DefaultConstruction_ZeroOrthoSize)
{
	FSceneView view;
	TestNear("EXPECT_FLOAT_EQ", view.OrthoWidth, 0.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", view.OrthoHeight, 0.0f, 1e-6f);
}

// ---------------------------------------------------------------
// FSceneView ??Perspective field defaults
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneViewTest, PerspectiveDefaults_FOV)
{
	FSceneView view;
	TestNear("EXPECT_FLOAT_EQ", view.FOV, 90.0f, 1e-6f);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneViewTest, PerspectiveDefaults_AspectRatio)
{
	FSceneView view;
	TestNear("EXPECT_FLOAT_EQ", view.AspectRatio, 16.0f / 9.0f, 1e-6f);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneViewTest, PerspectiveDefaults_NearClip)
{
	FSceneView view;
	TestNear("EXPECT_FLOAT_EQ", view.NearClip, 0.1f, 1e-6f);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneViewTest, PerspectiveDefaults_FarClip)
{
	FSceneView view;
	TestNear("EXPECT_FLOAT_EQ", view.FarClip, 10000.0f, 1e-6f);
}

// ---------------------------------------------------------------
// FSceneView ??Mutability
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneViewTest, ViewTransform_Mutable)
{
	FSceneView view;
	view.ViewTransform = FTransform(FVector(10.0f, 20.0f, 0.0f));
	const auto& t = view.ViewTransform.GetTranslation();
	TestNear("EXPECT_FLOAT_EQ", t.X, 10.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", t.Y, 20.0f, 1e-6f);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneViewTest, ViewportRect_Mutable)
{
	FSceneView view;
	view.ViewportWidth = 120;
	view.ViewportHeight = 40;
	TestEqual("EXPECT_EQ", view.ViewportWidth, 120);
	TestEqual("EXPECT_EQ", view.ViewportHeight, 40);
}
