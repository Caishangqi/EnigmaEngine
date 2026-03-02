// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file BlendState.h
/// @brief Generic color blend operations, reusable by any renderer.

#include <cstdint>

namespace Enigma
{

/// Generic color blend operations (per-channel R, G, B with uint8_t clamping).
/// Shared by ASCII renderer and future DX12 renderer.
enum class EBlendOp : uint8_t
{
	Replace,    ///< dst = src (default)
	Add,        ///< dst = clamp(src + dst, 0, 255)
	Multiply,   ///< dst = (src * dst) / 255
	Custom      ///< dst = customFn(src, dst)
};

} // namespace Enigma
