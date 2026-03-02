// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file SceneView.h
/// @brief Generic scene view system supporting orthographic and perspective projection.

#include "EngineAPI.generated.h"
#include "Math/Transform.h"

#include <cstdint>

namespace Enigma
{

/// Camera projection mode.
enum class ECameraProjectionMode : uint8_t
{
	Orthographic,
	Perspective
};

/// Generic scene view -- consumed by any renderer implementation.
/// ASCII renderer reads GetTranslation() as 2D scroll offset.
/// Future DX12 renderer uses ToMatrix() for full view matrix.
struct FSceneView
{
	ECameraProjectionMode ProjectionMode = ECameraProjectionMode::Orthographic;

	/// Camera transform in world space.
	/// ASCII renderer: GetTranslation() = 2D scroll offset.
	/// DX12 renderer: ToMatrix() = full view matrix.
	FTransform ViewTransform;  // default: identity (origin 0,0,0)

	/// Viewport rectangle (screen-space).
	int32_t ViewportX = 0, ViewportY = 0;
	int32_t ViewportWidth = 0, ViewportHeight = 0;

	// --- Orthographic ---
	float OrthoWidth  = 0.0f;  ///< Visible world width (character cells for ASCII)
	float OrthoHeight = 0.0f;  ///< Visible world height

	// --- Perspective (reserved for future DX12) ---
	float FOV         = 90.0f;
	float AspectRatio = 16.0f / 9.0f;
	float NearClip    = 0.1f;
	float FarClip     = 10000.0f;
};

} // namespace Enigma
