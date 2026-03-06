// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file RandomStream.h
/// @brief Seeded, deterministic random number generator.

#include "CoreAPI.generated.h"

#include <cstdint>

namespace Enigma
{

struct FVector;

/// @brief Seeded, deterministic random number generator.
///
/// Uses Mersenne Twister (std::mt19937) internally.
/// UE equivalent: FRandomStream (Core/Public/Math/RandomStream.h).
/// UE uses a linear congruential generator; we use std::mt19937
/// for better random quality.
class CORE_API FRandomStream
{
public:
	/// @brief Default constructor. Uses a non-deterministic seed.
	FRandomStream();

	/// @brief Construct with a specific seed.
	explicit FRandomStream(int32_t seed);

	/// @brief Destructor.
	~FRandomStream();

	/// @brief Copy constructor.
	FRandomStream(const FRandomStream& Other);

	/// @brief Copy assignment.
	FRandomStream& operator=(const FRandomStream& Other);

	/// @brief Set seed and reset state.
	void Initialize(int32_t seed);

	/// @brief Reset to initial seed (reproduces same sequence).
	void Reset();

	/// @brief Generate a new non-deterministic seed.
	void GenerateNewSeed();

	/// @brief Get the seed this stream was initialized with.
	int32_t GetInitialSeed() const;

	/// @brief Get the current seed value.
	int32_t GetCurrentSeed() const;

	/// @brief Random float in [0, 1).
	float GetFraction();

	/// @brief Random uint32.
	uint32_t GetUnsignedInt();

	/// @brief Random int in [0, A). Returns 0 if A <= 0.
	int32_t RandHelper(int32_t A);

	/// @brief Random int in [min, max] inclusive.
	int32_t RandRange(int32_t min, int32_t max);

	/// @brief Random float in [min, max].
	float FRandRange(float min, float max);

	/// @brief Random unit vector (uniformly distributed on sphere).
	FVector GetUnitVector();

	/// @brief Random bool.
	bool RandBool();

private:
	int32_t InitialSeed;

	// Opaque generator state. Defined in .cpp to avoid
	// exposing <random> in the public header.
	struct FImpl;
	FImpl* Impl;
};

} // namespace Enigma