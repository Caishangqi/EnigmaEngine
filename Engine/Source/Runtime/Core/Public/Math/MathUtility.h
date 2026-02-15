// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file MathUtility.h
/// @brief Centralized math constants and utility functions.

#include "CoreAPI.generated.h"

#include <cmath>
#include <algorithm>
#include <limits>

namespace Enigma
{

/// @brief Centralized math constants and static utility functions.
///
/// FMath provides common mathematical operations used throughout the
/// engine. Pure arithmetic functions are constexpr; trigonometric
/// functions delegate to the standard library.
struct CORE_API FMath
{
	// -----------------------------------------------------------------
	// Constants
	// -----------------------------------------------------------------

	/// Pi (3.14159...).
	static constexpr float Pi = 3.14159265358979323846f;

	/// Half Pi (Pi / 2).
	static constexpr float HalfPi = 1.57079632679489661923f;

	/// Two Pi (2 * Pi).
	static constexpr float TwoPi = 6.28318530717958647692f;

	/// Very small number for floating-point tolerance checks.
	static constexpr float SmallNumber = 1.e-8f;

	/// Slightly larger tolerance for "close enough" comparisons.
	static constexpr float KindaSmallNumber = 1.e-4f;

	/// Very large number (near float max).
	static constexpr float BigNumber = 3.4e+38f;

	/// Machine epsilon for float.
	static constexpr float Epsilon = std::numeric_limits<float>::epsilon();

	/// Degrees-to-radians conversion factor.
	static constexpr float DegToRad = Pi / 180.0f;

	/// Radians-to-degrees conversion factor.
	static constexpr float RadToDeg = 180.0f / Pi;

	// -----------------------------------------------------------------
	// Trigonometry (delegate to <cmath>, not constexpr)
	// -----------------------------------------------------------------

	/// @brief Sine of Value (radians).
	static float Sin(float Value);

	/// @brief Cosine of Value (radians).
	static float Cos(float Value);

	/// @brief Tangent of Value (radians).
	static float Tan(float Value);

	/// @brief Arc sine. Result in radians [-Pi/2, Pi/2].
	static float Asin(float Value);

	/// @brief Arc cosine. Result in radians [0, Pi].
	static float Acos(float Value);

	/// @brief Two-argument arc tangent. Result in radians [-Pi, Pi].
	static float Atan2(float Y, float X);

	// -----------------------------------------------------------------
	// Common math (non-constexpr due to <cmath>)
	// -----------------------------------------------------------------

	/// @brief Square root of Value.
	static float Sqrt(float Value);

	/// @brief Inverse square root (1 / sqrt(Value)).
	static float InvSqrt(float Value);

	/// @brief Floor of Value.
	static float Floor(float Value);

	/// @brief Ceiling of Value.
	static float Ceil(float Value);

	/// @brief Floating-point modulo (wraps std::fmod).
	static float Fmod(float X, float Y);

	/// @brief Raise Base to the power of Exp (wraps std::pow).
	static float Pow(float Base, float Exp);

	// -----------------------------------------------------------------
	// Pure arithmetic (constexpr)
	// -----------------------------------------------------------------

	/// @brief Absolute value.
	static constexpr float Abs(float Value)
	{
		return Value < 0.0f ? -Value : Value;
	}

	/// @brief Square of Value (Value * Value).
	static constexpr float Square(float Value)
	{
		return Value * Value;
	}

	/// @brief Minimum of two values.
	template<typename T>
	static constexpr T Min(const T& A, const T& B)
	{
		return (A < B) ? A : B;
	}

	/// @brief Maximum of two values.
	template<typename T>
	static constexpr T Max(const T& A, const T& B)
	{
		return (A > B) ? A : B;
	}

	// -----------------------------------------------------------------
	// Interpolation & Clamping (constexpr)
	// -----------------------------------------------------------------

	/// @brief Clamp Value to [MinVal, MaxVal].
	template<typename T>
	static constexpr T Clamp(T Value, T MinVal, T MaxVal)
	{
		return (Value < MinVal) ? MinVal : (Value > MaxVal) ? MaxVal : Value;
	}

	/// @brief Linear interpolation: A + (B - A) * Alpha.
	template<typename T>
	static constexpr T Lerp(const T& A, const T& B, float Alpha)
	{
		return static_cast<T>(A + (B - A) * Alpha);
	}

	// -----------------------------------------------------------------
	// Conversion (constexpr)
	// -----------------------------------------------------------------

	/// @brief Convert degrees to radians.
	static constexpr float DegreesToRadians(float Degrees)
	{
		return Degrees * DegToRad;
	}

	/// @brief Convert radians to degrees.
	static constexpr float RadiansToDegrees(float Radians)
	{
		return Radians * RadToDeg;
	}

	// -----------------------------------------------------------------
	// Comparison (constexpr)
	// -----------------------------------------------------------------

	/// @brief Check if two floats are nearly equal within Tolerance.
	static constexpr bool IsNearlyEqual(float A, float B, float Tolerance = SmallNumber)
	{
		return Abs(A - B) <= Tolerance;
	}

	/// @brief Check if a float is nearly zero within Tolerance.
	static constexpr bool IsNearlyZero(float Value, float Tolerance = SmallNumber)
	{
		return Abs(Value) <= Tolerance;
	}

	// -----------------------------------------------------------------
	// Angle utilities
	// -----------------------------------------------------------------

	/// @brief Normalize angle to [-180, 180) range (degrees).
	static float NormalizeAngle(float Angle);

	/// @brief Normalize angle to [0, 360) range (degrees).
	static float ClampAngle(float Angle);
};

} // namespace Enigma
