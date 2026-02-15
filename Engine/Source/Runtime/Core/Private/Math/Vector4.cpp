// Copyright EnigmaEngine. All Rights Reserved.

/// @file Vector4.cpp
/// @brief Implementation of FVector4 non-constexpr functions.

#include "Math/Vector4.h"

namespace Enigma
{

float FVector4::Size() const
{
	return FMath::Sqrt(X * X + Y * Y + Z * Z + W * W);
}

float FVector4::Size3() const
{
	return FMath::Sqrt(X * X + Y * Y + Z * Z);
}

} // namespace Enigma
