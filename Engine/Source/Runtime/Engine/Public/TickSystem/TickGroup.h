// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include <cstdint>

// -------------------------------------------------------------
// ETickGroup
//
// Defines the ordered execution phases within a single frame.
// Tick groups execute in deterministic order with hard
// synchronization barriers between them.
//
//   TG_PreUpdate  -- input results, AI decisions
//   TG_Update     -- gameplay logic, movement, animation
//   TG_PostUpdate -- camera follow, UI sync, cleanup
// -------------------------------------------------------------

namespace Enigma
{

enum class ETickGroup : uint8_t
{
	TG_PreUpdate  = 0,
	TG_Update     = 1,
	TG_PostUpdate = 2,

	TG_Count           // Sentinel, not a valid group
};

} // namespace Enigma
