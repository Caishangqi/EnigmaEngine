// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputKeys.cpp
/// @brief Implementation of FKey, EKeys constants, and TranslateKeyCode.

#include "InputKeys.h"

#include <functional>

namespace Enigma
{

// -----------------------------------------------------------------
// FKey implementation
// -----------------------------------------------------------------

FKey::FKey(std::string InKeyName)
	: keyName(std::move(InKeyName))
{
}

const std::string& FKey::GetKeyName() const
{
	return keyName;
}

bool FKey::IsValid() const
{
	return !keyName.empty();
}

bool FKey::IsAxisKey() const
{
	return keyName == "MouseX" || keyName == "MouseY"
		|| keyName == "MouseWheelAxis"
		|| keyName == "Gamepad_LeftX" || keyName == "Gamepad_LeftY"
		|| keyName == "Gamepad_RightX" || keyName == "Gamepad_RightY"
		|| keyName == "Gamepad_LeftTriggerAxis"
		|| keyName == "Gamepad_RightTriggerAxis";
}

bool FKey::IsMouseKey() const
{
	return keyName == "LeftMouseButton"
		|| keyName == "RightMouseButton"
		|| keyName == "MiddleMouseButton"
		|| keyName == "MouseX"
		|| keyName == "MouseY"
		|| keyName == "MouseWheelAxis";
}

bool FKey::IsGamepadKey() const
{
	return keyName.find("Gamepad_") == 0;
}

bool FKey::IsModifierKey() const
{
	return keyName == "LeftShift" || keyName == "RightShift"
		|| keyName == "LeftControl" || keyName == "RightControl"
		|| keyName == "LeftAlt" || keyName == "RightAlt"
		|| keyName == "CapsLock";
}

bool FKey::IsDigitalKey() const
{
	return !IsAxisKey();
}

bool FKey::operator==(const FKey& Other) const
{
	return keyName == Other.keyName;
}

bool FKey::operator!=(const FKey& Other) const
{
	return keyName != Other.keyName;
}

size_t FKey::Hash::operator()(const FKey& Key) const
{
	return std::hash<std::string>{}(Key.keyName);
}

// -----------------------------------------------------------------
// EKeys static constants
// -----------------------------------------------------------------

namespace EKeys
{
	const FKey None;

	// Letters
	const FKey A(std::string("A")), B(std::string("B")), C(std::string("C"));
	const FKey D(std::string("D")), E(std::string("E")), F(std::string("F"));
	const FKey G(std::string("G")), H(std::string("H")), I(std::string("I"));
	const FKey J(std::string("J")), K(std::string("K")), L(std::string("L"));
	const FKey M(std::string("M")), N(std::string("N")), O(std::string("O"));
	const FKey P(std::string("P")), Q(std::string("Q")), R(std::string("R"));
	const FKey S(std::string("S")), T(std::string("T")), U(std::string("U"));
	const FKey V(std::string("V")), W(std::string("W")), X(std::string("X"));
	const FKey Y(std::string("Y")), Z(std::string("Z"));

	// Digits
	const FKey Zero(std::string("Zero")), One(std::string("One"));
	const FKey Two(std::string("Two")), Three(std::string("Three"));
	const FKey Four(std::string("Four")), Five(std::string("Five"));
	const FKey Six(std::string("Six")), Seven(std::string("Seven"));
	const FKey Eight(std::string("Eight")), Nine(std::string("Nine"));

	// Modifiers
	const FKey LeftShift(std::string("LeftShift"));
	const FKey RightShift(std::string("RightShift"));
	const FKey LeftControl(std::string("LeftControl"));
	const FKey RightControl(std::string("RightControl"));
	const FKey LeftAlt(std::string("LeftAlt"));
	const FKey RightAlt(std::string("RightAlt"));
	const FKey CapsLock(std::string("CapsLock"));

	// Navigation
	const FKey Escape(std::string("Escape"));
	const FKey Tab(std::string("Tab"));
	const FKey SpaceBar(std::string("SpaceBar"));
	const FKey Enter(std::string("Enter"));
	const FKey BackSpace(std::string("BackSpace"));
	const FKey Delete(std::string("Delete"));
	const FKey Up(std::string("Up")), Down(std::string("Down"));
	const FKey Left(std::string("Left")), Right(std::string("Right"));

	// Function keys
	const FKey F1(std::string("F1")), F2(std::string("F2"));
	const FKey F3(std::string("F3")), F4(std::string("F4"));
	const FKey F5(std::string("F5")), F6(std::string("F6"));
	const FKey F7(std::string("F7")), F8(std::string("F8"));
	const FKey F9(std::string("F9")), F10(std::string("F10"));
	const FKey F11(std::string("F11")), F12(std::string("F12"));

	// Mouse
	const FKey LeftMouseButton(std::string("LeftMouseButton"));
	const FKey RightMouseButton(std::string("RightMouseButton"));
	const FKey MiddleMouseButton(std::string("MiddleMouseButton"));
	const FKey MouseX(std::string("MouseX"));
	const FKey MouseY(std::string("MouseY"));
	const FKey MouseWheelAxis(std::string("MouseWheelAxis"));

	// Gamepad (stubs)
	const FKey Gamepad_FaceButton_Top(std::string("Gamepad_FaceButton_Top"));
	const FKey Gamepad_FaceButton_Bottom(std::string("Gamepad_FaceButton_Bottom"));
	const FKey Gamepad_FaceButton_Left(std::string("Gamepad_FaceButton_Left"));
	const FKey Gamepad_FaceButton_Right(std::string("Gamepad_FaceButton_Right"));
	const FKey Gamepad_LeftX(std::string("Gamepad_LeftX"));
	const FKey Gamepad_LeftY(std::string("Gamepad_LeftY"));
	const FKey Gamepad_RightX(std::string("Gamepad_RightX"));
	const FKey Gamepad_RightY(std::string("Gamepad_RightY"));
	const FKey Gamepad_LeftTriggerAxis(std::string("Gamepad_LeftTriggerAxis"));
	const FKey Gamepad_RightTriggerAxis(std::string("Gamepad_RightTriggerAxis"));

} // namespace EKeys

// -----------------------------------------------------------------
// TranslateKeyCode (Windows VK codes)
// -----------------------------------------------------------------

FKey TranslateKeyCode(int32_t platformKeyCode)
{
	// Windows Virtual Key codes
	switch (platformKeyCode)
	{
	// Letters (VK_A = 0x41 .. VK_Z = 0x5A)
	case 0x41: return EKeys::A;  case 0x42: return EKeys::B;
	case 0x43: return EKeys::C;  case 0x44: return EKeys::D;
	case 0x45: return EKeys::E;  case 0x46: return EKeys::F;
	case 0x47: return EKeys::G;  case 0x48: return EKeys::H;
	case 0x49: return EKeys::I;  case 0x4A: return EKeys::J;
	case 0x4B: return EKeys::K;  case 0x4C: return EKeys::L;
	case 0x4D: return EKeys::M;  case 0x4E: return EKeys::N;
	case 0x4F: return EKeys::O;  case 0x50: return EKeys::P;
	case 0x51: return EKeys::Q;  case 0x52: return EKeys::R;
	case 0x53: return EKeys::S;  case 0x54: return EKeys::T;
	case 0x55: return EKeys::U;  case 0x56: return EKeys::V;
	case 0x57: return EKeys::W;  case 0x58: return EKeys::X;
	case 0x59: return EKeys::Y;  case 0x5A: return EKeys::Z;

	// Digits (VK_0 = 0x30 .. VK_9 = 0x39)
	case 0x30: return EKeys::Zero;  case 0x31: return EKeys::One;
	case 0x32: return EKeys::Two;   case 0x33: return EKeys::Three;
	case 0x34: return EKeys::Four;  case 0x35: return EKeys::Five;
	case 0x36: return EKeys::Six;   case 0x37: return EKeys::Seven;
	case 0x38: return EKeys::Eight; case 0x39: return EKeys::Nine;

	// Navigation
	case 0x1B: return EKeys::Escape;
	case 0x09: return EKeys::Tab;
	case 0x20: return EKeys::SpaceBar;
	case 0x0D: return EKeys::Enter;
	case 0x08: return EKeys::BackSpace;
	case 0x2E: return EKeys::Delete;
	case 0x26: return EKeys::Up;
	case 0x28: return EKeys::Down;
	case 0x25: return EKeys::Left;
	case 0x27: return EKeys::Right;

	// Modifiers
	case 0xA0: return EKeys::LeftShift;
	case 0xA1: return EKeys::RightShift;
	case 0xA2: return EKeys::LeftControl;
	case 0xA3: return EKeys::RightControl;
	case 0xA4: return EKeys::LeftAlt;
	case 0xA5: return EKeys::RightAlt;
	case 0x14: return EKeys::CapsLock;

	// Function keys (VK_F1 = 0x70 .. VK_F12 = 0x7B)
	case 0x70: return EKeys::F1;  case 0x71: return EKeys::F2;
	case 0x72: return EKeys::F3;  case 0x73: return EKeys::F4;
	case 0x74: return EKeys::F5;  case 0x75: return EKeys::F6;
	case 0x76: return EKeys::F7;  case 0x77: return EKeys::F8;
	case 0x78: return EKeys::F9;  case 0x79: return EKeys::F10;
	case 0x7A: return EKeys::F11; case 0x7B: return EKeys::F12;

	default: return EKeys::None;
	}
}

} // namespace Enigma
