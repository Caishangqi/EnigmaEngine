// Copyright EnigmaEngine. All Rights Reserved.

/// @file SceneViewTest.cpp
/// @brief Unit tests for FSceneView and ECameraProjectionMode.

#include <gtest/gtest.h>

#include "SceneView/SceneView.h"
#include "Math/Vector.h"

using namespace Enigma;

// ---------------------------------------------------------------
// ECameraProjectionMode
// ---------------------------------------------------------------

TEST(CameraProjectionModeTest, EnumValues)
{
	// Verify enum values exist and are distinct.
	auto ortho = ECameraProjectionMode::Orthographic;
	auto persp = ECameraProjectionMode::Perspective;
	EXPECT_NE(static_cast<uint8_t>(ortho), static_cast<uint8_t>(persp));
}

TEST(CameraProjectionModeTest, Orthographic_IsZero)
{
	EXPECT_EQ(static_cast<uint8_t>(ECameraProjectionMode::Orthographic), 0);
}

TEST(CameraProjectionModeTest, Perspective_IsOne)
{
	EXPECT_EQ(static_cast<uint8_t>(ECameraProjectionMode::Perspective), 1);
}

// ---------------------------------------------------------------
// FSceneView — Default construction
// ---------------------------------------------------------------

TEST(SceneViewTest, DefaultConstruction_Orthographic)
{
	FSceneView view;
	EXPECT_EQ(view.ProjectionMode, ECameraProjectionMode::Orthographic);
}

TEST(SceneViewTest, DefaultConstruction_IdentityTransform)
{
	FSceneView view;
	const auto& translation = view.ViewTransform.GetTranslation();
	EXPECT_FLOAT_EQ(translation.X, 0.0f);
	EXPECT_FLOAT_EQ(translation.Y, 0.0f);
	EXPECT_FLOAT_EQ(translation.Z, 0.0f);
}

TEST(SceneViewTest, DefaultConstruction_ZeroViewport)
{
	FSceneView view;
	EXPECT_EQ(view.ViewportX, 0);
	EXPECT_EQ(view.ViewportY, 0);
	EXPECT_EQ(view.ViewportWidth, 0);
	EXPECT_EQ(view.ViewportHeight, 0);
}

TEST(SceneViewTest, DefaultConstruction_ZeroOrthoSize)
{
	FSceneView view;
	EXPECT_FLOAT_EQ(view.OrthoWidth, 0.0f);
	EXPECT_FLOAT_EQ(view.OrthoHeight, 0.0f);
}

// ---------------------------------------------------------------
// FSceneView — Perspective field defaults
// ---------------------------------------------------------------

TEST(SceneViewTest, PerspectiveDefaults_FOV)
{
	FSceneView view;
	EXPECT_FLOAT_EQ(view.FOV, 90.0f);
}

TEST(SceneViewTest, PerspectiveDefaults_AspectRatio)
{
	FSceneView view;
	EXPECT_FLOAT_EQ(view.AspectRatio, 16.0f / 9.0f);
}

TEST(SceneViewTest, PerspectiveDefaults_NearClip)
{
	FSceneView view;
	EXPECT_FLOAT_EQ(view.NearClip, 0.1f);
}

TEST(SceneViewTest, PerspectiveDefaults_FarClip)
{
	FSceneView view;
	EXPECT_FLOAT_EQ(view.FarClip, 10000.0f);
}

// ---------------------------------------------------------------
// FSceneView — Mutability
// ---------------------------------------------------------------

TEST(SceneViewTest, ViewTransform_Mutable)
{
	FSceneView view;
	view.ViewTransform = FTransform(FVector(10.0f, 20.0f, 0.0f));
	const auto& t = view.ViewTransform.GetTranslation();
	EXPECT_FLOAT_EQ(t.X, 10.0f);
	EXPECT_FLOAT_EQ(t.Y, 20.0f);
}

TEST(SceneViewTest, ViewportRect_Mutable)
{
	FSceneView view;
	view.ViewportWidth = 120;
	view.ViewportHeight = 40;
	EXPECT_EQ(view.ViewportWidth, 120);
	EXPECT_EQ(view.ViewportHeight, 40);
}
