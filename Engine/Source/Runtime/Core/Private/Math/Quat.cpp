// Copyright EnigmaEngine. All Rights Reserved.

/// @file Quat.cpp
/// @brief Implementation of FQuat non-constexpr functions.

#include "Math/Quat.h"
#include "Math/Matrix.h"
#include "Math/Rotator.h"

namespace Enigma
{

// -----------------------------------------------------------------
// Constants
// -----------------------------------------------------------------

const FQuat FQuat::Identity(0.0f, 0.0f, 0.0f, 1.0f);

// -----------------------------------------------------------------
// Constructors
// -----------------------------------------------------------------

FQuat::FQuat(const FVector& Axis, float AngleRad)
{
	const float Half = AngleRad * 0.5f;
	const float S = FMath::Sin(Half);
	const float C = FMath::Cos(Half);
	X = Axis.X * S;
	Y = Axis.Y * S;
	Z = Axis.Z * S;
	W = C;
}

// -----------------------------------------------------------------
// Operators
// -----------------------------------------------------------------

FQuat FQuat::operator*(const FQuat& Q) const
{
	// Hamilton product.
	return FQuat(
		W * Q.X + X * Q.W + Y * Q.Z - Z * Q.Y,
		W * Q.Y - X * Q.Z + Y * Q.W + Z * Q.X,
		W * Q.Z + X * Q.Y - Y * Q.X + Z * Q.W,
		W * Q.W - X * Q.X - Y * Q.Y - Z * Q.Z);
}

FVector FQuat::operator*(const FVector& V) const
{
	return RotateVector(V);
}

// -----------------------------------------------------------------
// Operations
// -----------------------------------------------------------------

FVector FQuat::RotateVector(const FVector& V) const
{
	// Optimized q*v*q^-1: t = 2*(q.xyz x v), result = v + w*t + (q.xyz x t)
	const FVector Q(X, Y, Z);
	const FVector T = 2.0f * FVector::CrossProduct(Q, V);
	return V + W * T + FVector::CrossProduct(Q, T);
}

FVector FQuat::UnrotateVector(const FVector& V) const
{
	// Apply conjugate rotation: (-q.xyz, q.w).RotateVector(V)
	const FVector Q(-X, -Y, -Z);
	const FVector T = 2.0f * FVector::CrossProduct(Q, V);
	return V + W * T + FVector::CrossProduct(Q, T);
}

FQuat FQuat::GetNormalized() const
{
	const float Sq = SizeSquared();
	if (Sq > FMath::SmallNumber)
	{
		const float Scale = FMath::InvSqrt(Sq);
		return FQuat(X * Scale, Y * Scale, Z * Scale, W * Scale);
	}
	return Identity;
}

bool FQuat::Normalize(float Tolerance)
{
	const float Sq = SizeSquared();
	if (Sq > Tolerance)
	{
		const float Scale = FMath::InvSqrt(Sq);
		X *= Scale;
		Y *= Scale;
		Z *= Scale;
		W *= Scale;
		return true;
	}
	return false;
}

float FQuat::Size() const
{
	return FMath::Sqrt(X * X + Y * Y + Z * Z + W * W);
}

FQuat FQuat::GetInverse() const
{
	const float Sq = SizeSquared();
	if (Sq > FMath::SmallNumber)
	{
		const float InvSq = 1.0f / Sq;
		return FQuat(-X * InvSq, -Y * InvSq, -Z * InvSq, W * InvSq);
	}
	return Identity;
}

FMatrix FQuat::ToMatrix() const
{
	return FMatrix::MakeRotation(*this);
}

FRotator FQuat::ToRotator() const
{
	// Extract Euler angles (YXZ intrinsic order) from quaternion.
	// Pitch = rotation around X, Yaw = around Y, Roll = around Z.
	const float SinPitch = 2.0f * (W * X - Y * Z);

	float Pitch, Yaw, Roll;

	if (FMath::Abs(SinPitch) >= 1.0f)
	{
		// Gimbal lock at +/-90 degrees.
		Pitch = (SinPitch > 0.0f) ? 90.0f : -90.0f;
		Yaw = FMath::Atan2(2.0f * (X * Z + W * Y), 1.0f - 2.0f * (X * X + Y * Y))
			* FMath::RadToDeg;
		Roll = 0.0f;
	}
	else
	{
		Pitch = FMath::Asin(SinPitch) * FMath::RadToDeg;
		Yaw = FMath::Atan2(2.0f * (X * Z + W * Y), 1.0f - 2.0f * (X * X + Y * Y))
			* FMath::RadToDeg;
		Roll = FMath::Atan2(2.0f * (X * Y + W * Z), 1.0f - 2.0f * (X * X + Z * Z))
			* FMath::RadToDeg;
	}

	return FRotator(Pitch, Yaw, Roll);
}

// -----------------------------------------------------------------
// Interpolation
// -----------------------------------------------------------------

FQuat FQuat::Slerp(const FQuat& A, const FQuat& B, float Alpha)
{
	float CosAngle = A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W;

	// Ensure shortest path.
	FQuat B2 = B;
	if (CosAngle < 0.0f)
	{
		B2 = FQuat(-B.X, -B.Y, -B.Z, -B.W);
		CosAngle = -CosAngle;
	}

	float ScaleA, ScaleB;

	if (CosAngle > 0.9995f)
	{
		// Nearly parallel -- fall back to normalized linear interpolation.
		ScaleA = 1.0f - Alpha;
		ScaleB = Alpha;
	}
	else
	{
		const float Angle = FMath::Acos(CosAngle);
		const float SinAngle = FMath::Sin(Angle);
		ScaleA = FMath::Sin((1.0f - Alpha) * Angle) / SinAngle;
		ScaleB = FMath::Sin(Alpha * Angle) / SinAngle;
	}

	FQuat Result(
		ScaleA * A.X + ScaleB * B2.X,
		ScaleA * A.Y + ScaleB * B2.Y,
		ScaleA * A.Z + ScaleB * B2.Z,
		ScaleA * A.W + ScaleB * B2.W);

	// Normalize for the Nlerp fallback path.
	if (CosAngle > 0.9995f)
	{
		Result.Normalize();
	}

	return Result;
}

} // namespace Enigma
