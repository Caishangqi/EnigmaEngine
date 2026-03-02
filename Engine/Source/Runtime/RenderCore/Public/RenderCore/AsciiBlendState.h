// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file AsciiBlendState.h
/// @brief ASCII-specific blend state with Fluent API, write mask, and presets.

#include "RenderCoreAPI.generated.h"
#include "RenderCore/BlendState.h"
#include "Math/Color.h"

#include <cstdint>
#include <functional>

namespace Enigma
{

/// Character blend operations (ASCII-specific - no GPU equivalent).
enum class ECharBlendOp : uint8_t
{
	Replace,      ///< Always use new character (default)
	KeepNonSpace, ///< Only overwrite if dst is space (' ')
	Custom        ///< dst = customFn(src, dst)
};

/// ASCII cell blend state - Fluent API with chainable setters and write mask.
/// Presets serve as customizable starting points via method chaining.
struct FAsciiBlendState
{
	// --- Blend operations ---
	EBlendOp     ForegroundOp = EBlendOp::Replace;
	EBlendOp     BackgroundOp = EBlendOp::Replace;
	ECharBlendOp CharacterOp  = ECharBlendOp::Replace;

	// --- Write mask (inspired by UE's EColorWriteMask) ---
	bool bWriteCharacter  = true;
	bool bWriteForeground = true;
	bool bWriteBackground = true;

	// --- Custom blend functions ---
	std::function<FColor(FColor src, FColor dst)> CustomForegroundFn;
	std::function<FColor(FColor src, FColor dst)> CustomBackgroundFn;
	std::function<char(char src, char dst)>       CustomCharacterFn;

	// --- Chainable setters (Fluent API) ---

	/// Set foreground color blend operation.
	FAsciiBlendState& WithForeground(EBlendOp op)
	{
		ForegroundOp = op;
		return *this;
	}

	/// Set background color blend operation.
	FAsciiBlendState& WithBackground(EBlendOp op)
	{
		BackgroundOp = op;
		return *this;
	}
	/// Set character blend operation.
	FAsciiBlendState& WithCharacter(ECharBlendOp op)
	{
		CharacterOp = op;
		return *this;
	}

	/// Set write mask for character, foreground, and background channels.
	FAsciiBlendState& WithWriteMask(bool ch, bool fg, bool bg)
	{
		bWriteCharacter = ch;
		bWriteForeground = fg;
		bWriteBackground = bg;
		return *this;
	}

	/// Set custom foreground blend function (also sets ForegroundOp to Custom).
	FAsciiBlendState& WithCustomForeground(std::function<FColor(FColor, FColor)> fn)
	{
		ForegroundOp = EBlendOp::Custom;
		CustomForegroundFn = std::move(fn);
		return *this;
	}

	/// Set custom background blend function (also sets BackgroundOp to Custom).
	FAsciiBlendState& WithCustomBackground(std::function<FColor(FColor, FColor)> fn)
	{
		BackgroundOp = EBlendOp::Custom;
		CustomBackgroundFn = std::move(fn);
		return *this;
	}

	/// Set custom character blend function (also sets CharacterOp to Custom).
	FAsciiBlendState& WithCustomCharacter(std::function<char(char, char)> fn)
	{
		CharacterOp = ECharBlendOp::Custom;
		CustomCharacterFn = std::move(fn);
		return *this;
	}
	// --- Presets (customizable starting points) ---

	/// Default opaque blend: all Replace, all write masks true.
	static FAsciiBlendState Opaque()
	{
		return FAsciiBlendState{};
	}

	/// Transparent blend: KeepNonSpace for character channel.
	static FAsciiBlendState Transparent()
	{
		FAsciiBlendState s;
		s.CharacterOp = ECharBlendOp::KeepNonSpace;
		return s;
	}

	/// Additive blend: Add for foreground and background channels.
	static FAsciiBlendState Additive()
	{
		FAsciiBlendState s;
		s.ForegroundOp = EBlendOp::Add;
		s.BackgroundOp = EBlendOp::Add;
		return s;
	}

	// --- Equality (enum fields + write mask; custom fns compared by nullptr state) ---

	bool operator==(const FAsciiBlendState& other) const
	{
		return ForegroundOp == other.ForegroundOp
			&& BackgroundOp == other.BackgroundOp
			&& CharacterOp  == other.CharacterOp
			&& bWriteCharacter  == other.bWriteCharacter
			&& bWriteForeground == other.bWriteForeground
			&& bWriteBackground == other.bWriteBackground
			&& static_cast<bool>(CustomForegroundFn) == static_cast<bool>(other.CustomForegroundFn)
			&& static_cast<bool>(CustomBackgroundFn) == static_cast<bool>(other.CustomBackgroundFn)
			&& static_cast<bool>(CustomCharacterFn)  == static_cast<bool>(other.CustomCharacterFn);
	}

	bool operator!=(const FAsciiBlendState& other) const { return !(*this == other); }
};

} // namespace Enigma
