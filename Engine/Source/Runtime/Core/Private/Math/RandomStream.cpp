// Copyright EnigmaEngine. All Rights Reserved.

/// @file RandomStream.cpp
/// @brief Implementation of FRandomStream.

#include "Math/RandomStream.h"
#include "Math/MathUtility.h"
#include "Math/Vector.h"

#include <random>

namespace Enigma
{

// -----------------------------------------------------------------
// Pimpl: hides <random> from the public header
// -----------------------------------------------------------------

struct FRandomStream::FImpl
{
	std::mt19937 Generator;
};

// -----------------------------------------------------------------
// Construction
// -----------------------------------------------------------------

FRandomStream::FRandomStream()
	: InitialSeed(0)
	, Impl(new FImpl)
{
	GenerateNewSeed();
}

FRandomStream::FRandomStream(int32_t seed)
	: InitialSeed(seed)
	, Impl(new FImpl)
{
	Impl->Generator.seed(static_cast<uint32_t>(seed));
}

FRandomStream::~FRandomStream()
{
	delete Impl;
}

FRandomStream::FRandomStream(const FRandomStream& Other)
	: InitialSeed(Other.InitialSeed)
	, Impl(new FImpl)
{
	Impl->Generator = Other.Impl->Generator;
}

FRandomStream& FRandomStream::operator=(const FRandomStream& Other)
{
	if (this != &Other)
	{
		InitialSeed = Other.InitialSeed;
		Impl->Generator = Other.Impl->Generator;
	}
	return *this;
}

// -----------------------------------------------------------------
// Seed management
// -----------------------------------------------------------------

void FRandomStream::Initialize(int32_t seed)
{
	InitialSeed = seed;
	Impl->Generator.seed(static_cast<uint32_t>(seed));
}

void FRandomStream::Reset()
{
	Impl->Generator.seed(static_cast<uint32_t>(InitialSeed));
}

void FRandomStream::GenerateNewSeed()
{
	std::random_device rd;
	InitialSeed = static_cast<int32_t>(rd());
	Impl->Generator.seed(static_cast<uint32_t>(InitialSeed));
}

int32_t FRandomStream::GetInitialSeed() const
{
	return InitialSeed;
}

int32_t FRandomStream::GetCurrentSeed() const
{
	return InitialSeed;
}

// -----------------------------------------------------------------
// Random value generation
// -----------------------------------------------------------------

float FRandomStream::GetFraction()
{
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);
	return dist(Impl->Generator);
}

uint32_t FRandomStream::GetUnsignedInt()
{
	return Impl->Generator();
}

int32_t FRandomStream::RandHelper(int32_t A)
{
	if (A <= 0)
	{
		return 0;
	}
	std::uniform_int_distribution<int32_t> dist(0, A - 1);
	return dist(Impl->Generator);
}

int32_t FRandomStream::RandRange(int32_t min, int32_t max)
{
	std::uniform_int_distribution<int32_t> dist(min, max);
	return dist(Impl->Generator);
}

float FRandomStream::FRandRange(float min, float max)
{
	std::uniform_real_distribution<float> dist(min, max);
	return dist(Impl->Generator);
}

FVector FRandomStream::GetUnitVector()
{
	// Uniform distribution on sphere via spherical coordinates
	float theta = GetFraction() * FMath::TwoPi;
	float z = GetFraction() * 2.0f - 1.0f;
	float r = FMath::Sqrt(1.0f - z * z);
	return FVector(r * FMath::Cos(theta), r * FMath::Sin(theta), z);
}

bool FRandomStream::RandBool()
{
	return (GetUnsignedInt() & 1u) != 0;
}

} // namespace Enigma
