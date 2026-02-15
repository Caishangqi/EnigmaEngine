// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file MathFwd.h
/// @brief Forward declarations for all Core math types.
///
/// Include this header when you need to reference math types by
/// pointer or reference without pulling in their full definitions.
/// This avoids circular dependencies between math headers.

namespace Enigma
{
	/// 2D floating-point vector (X, Y).
	struct FVector2D;

	/// 3D floating-point vector (X, Y, Z).
	struct FVector;

	/// 4D floating-point vector (X, Y, Z, W).
	struct FVector4;

	/// 3D integer vector (X, Y, Z) for voxel coordinates.
	struct FIntVector;

	/// 4x4 row-major floating-point matrix.
	struct FMatrix;

	/// Quaternion rotation (X, Y, Z, W).
	struct FQuat;

	/// Euler angle rotation in degrees (Pitch, Yaw, Roll).
	struct FRotator;

	/// Composite transform: translation + rotation + scale.
	struct FTransform;

	/// Linear-space floating-point color (R, G, B, A).
	struct FLinearColor;

	/// 8-bit sRGB color (R, G, B, A).
	struct FColor;

	/// Centralized math constants and utility functions.
	struct FMath;

} // namespace Enigma
