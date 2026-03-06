// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file InputKeys.h
/// @brief Unified physical input key type covering keyboard, mouse, and gamepad.

#include "CoreAPI.generated.h"

#include <cstdint>
#include <string>

// ENHANCEDINPUT_API macro: use the same pattern as other modules.
// When building EnhancedInput DLL, ENHANCEDINPUT_EXPORTS is defined.
#include "HAL/Platform.h"

#ifdef ENHANCEDINPUT_STATIC_DEFINE
	#define ENHANCEDINPUT_API
#elif defined(ENHANCEDINPUT_EXPORTS)
	#define ENHANCEDINPUT_API DLLEXPORT
#else
	#define ENHANCEDINPUT_API DLLIMPORT
#endif

namespace Enigma
{

/// @brief Physical input key identifier.
///
/// UE equivalent: FKey (InputCore/Classes/InputCoreTypes.h)
/// UE uses FName internally; we use std::string now, will optimize to FName later.
struct ENHANCEDINPUT_API FKey
{
	FKey() = default;
	explicit FKey(std::string InKeyName);

	const std::string& GetKeyName() const;
	bool IsValid() const;

	/// Key metadata queries (UE equivalent: FKeyDetails properties).
	bool IsAxisKey() const;
	bool IsMouseKey() const;
	bool IsGamepadKey() const;
	bool IsModifierKey() const;
	bool IsDigitalKey() const;

	bool operator==(const FKey& Other) const;
	bool operator!=(const FKey& Other) const;

	/// Hash support for unordered containers.
	struct Hash { size_t operator()(const FKey& Key) const; };

private:
	std::string keyName;  // future: replace with FName (hashed string)
};

/// Static key constants (UE equivalent: EKeys namespace).
namespace EKeys
{
	// None (invalid key)
	extern ENHANCEDINPUT_API const FKey None;

	// Keyboard - Letters (A-Z)
	extern ENHANCEDINPUT_API const FKey A, B, C, D, E, F, G, H, I, J, K, L, M;
	extern ENHANCEDINPUT_API const FKey N, O, P, Q, R, S, T, U, V, W, X, Y, Z;

	// Keyboard - Digits
	extern ENHANCEDINPUT_API const FKey Zero, One, Two, Three, Four;
	extern ENHANCEDINPUT_API const FKey Five, Six, Seven, Eight, Nine;

	// Keyboard - Modifiers
	extern ENHANCEDINPUT_API const FKey LeftShift, RightShift;
	extern ENHANCEDINPUT_API const FKey LeftControl, RightControl;
	extern ENHANCEDINPUT_API const FKey LeftAlt, RightAlt;
	extern ENHANCEDINPUT_API const FKey CapsLock;

	// Keyboard - Navigation
	extern ENHANCEDINPUT_API const FKey Escape, Tab, SpaceBar, Enter, BackSpace, Delete;
	extern ENHANCEDINPUT_API const FKey Up, Down, Left, Right;

	// Keyboard - Function keys
	extern ENHANCEDINPUT_API const FKey F1, F2, F3, F4, F5, F6;
	extern ENHANCEDINPUT_API const FKey F7, F8, F9, F10, F11, F12;

	// Mouse
	extern ENHANCEDINPUT_API const FKey LeftMouseButton, RightMouseButton, MiddleMouseButton;
	extern ENHANCEDINPUT_API const FKey MouseX, MouseY, MouseWheelAxis;

	// Gamepad (stubs)
	extern ENHANCEDINPUT_API const FKey Gamepad_FaceButton_Top, Gamepad_FaceButton_Bottom;
	extern ENHANCEDINPUT_API const FKey Gamepad_FaceButton_Left, Gamepad_FaceButton_Right;
	extern ENHANCEDINPUT_API const FKey Gamepad_LeftX, Gamepad_LeftY;
	extern ENHANCEDINPUT_API const FKey Gamepad_RightX, Gamepad_RightY;
	extern ENHANCEDINPUT_API const FKey Gamepad_LeftTriggerAxis, Gamepad_RightTriggerAxis;
} // namespace EKeys

/// Translate platform virtual key code to FKey.
ENHANCEDINPUT_API FKey TranslateKeyCode(int32_t platformKeyCode);

} // namespace Enigma
