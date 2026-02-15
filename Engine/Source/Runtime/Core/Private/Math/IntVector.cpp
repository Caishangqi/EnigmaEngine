// Copyright EnigmaEngine. All Rights Reserved.

/// @file IntVector.cpp
/// @brief Implementation of FIntVector conversion methods and static constants.

#include "Math/IntVector.h"
#include "Math/Vector.h"

#include <cmath>

namespace Enigma
{

// -----------------------------------------------------------------
// Conversion
// -----------------------------------------------------------------

FVector FIntVector::ToFloat() const
{
	return FVector(
		static_cast<float>(X),
		static_cast<float>(Y),
		static_cast<float>(Z)
	);
}

FIntVector FIntVector::FromFloat(const FVector& V)
{
	return FIntVector(
		static_cast<int32_t>(std::floor(V.X)),
		static_cast<int32_t>(std::floor(V.Y)),
		static_cast<int32_t>(std::floor(V.Z))
	);
}

// -----------------------------------------------------------------
// Predefined constants
// -----------------------------------------------------------------

const FIntVector FIntVector::ZeroValue = FIntVector(0, 0, 0);
const FIntVector FIntVector::OneValue  = FIntVector(1, 1, 1);

} // namespace Enigma
