// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputMessageBridge.cpp
/// @brief Implementation of FInputMessageBridge.

#include "InputMessageBridge.h"
#include "InputSubsystem.h"

namespace Enigma
{

FInputMessageBridge::FInputMessageBridge(
	FInputSubsystem* inputSubsystem,
	FGenericApplicationMessageHandler* innerHandler)
	: InputSubsystem(inputSubsystem)
	, InnerHandler(innerHandler)
{
}

// -----------------------------------------------------------------
// Keyboard
// -----------------------------------------------------------------

bool FInputMessageBridge::OnKeyDown(int32_t keyCode, uint32_t charCode, bool bIsRepeat)
{
	if (InputSubsystem)
	{
		FKey key = TranslateKeyCode(keyCode);
		if (key.IsValid())
		{
			InputSubsystem->SetKeyState(key, true);
		}
	}

	if (InnerHandler)
	{
		return InnerHandler->OnKeyDown(keyCode, charCode, bIsRepeat);
	}
	return false;
}

bool FInputMessageBridge::OnKeyUp(int32_t keyCode, uint32_t charCode, bool bIsRepeat)
{
	if (InputSubsystem)
	{
		FKey key = TranslateKeyCode(keyCode);
		if (key.IsValid())
		{
			InputSubsystem->SetKeyState(key, false);
		}
	}

	if (InnerHandler)
	{
		return InnerHandler->OnKeyUp(keyCode, charCode, bIsRepeat);
	}
	return false;
}

// -----------------------------------------------------------------
// Mouse buttons
// -----------------------------------------------------------------

bool FInputMessageBridge::OnMouseDown(EMouseButton button, float cursorX, float cursorY)
{
	if (InputSubsystem)
	{
		FKey key = MouseButtonToKey(button);
		if (key.IsValid())
		{
			InputSubsystem->SetKeyState(key, true);
		}
	}

	if (InnerHandler)
	{
		return InnerHandler->OnMouseDown(button, cursorX, cursorY);
	}
	return false;
}

bool FInputMessageBridge::OnMouseUp(EMouseButton button, float cursorX, float cursorY)
{
	if (InputSubsystem)
	{
		FKey key = MouseButtonToKey(button);
		if (key.IsValid())
		{
			InputSubsystem->SetKeyState(key, false);
		}
	}

	if (InnerHandler)
	{
		return InnerHandler->OnMouseUp(button, cursorX, cursorY);
	}
	return false;
}

// -----------------------------------------------------------------
// Mouse axes
// -----------------------------------------------------------------

bool FInputMessageBridge::OnMouseMove(float cursorX, float cursorY)
{
	if (InputSubsystem)
	{
		InputSubsystem->SetAxisValue(EKeys::MouseX, cursorX);
		InputSubsystem->SetAxisValue(EKeys::MouseY, cursorY);
	}

	if (InnerHandler)
	{
		return InnerHandler->OnMouseMove(cursorX, cursorY);
	}
	return false;
}

bool FInputMessageBridge::OnMouseWheel(float delta, float cursorX, float cursorY)
{
	if (InputSubsystem)
	{
		InputSubsystem->SetAxisValue(EKeys::MouseWheelAxis, delta);
	}

	if (InnerHandler)
	{
		return InnerHandler->OnMouseWheel(delta, cursorX, cursorY);
	}
	return false;
}

// -----------------------------------------------------------------
// Window events (forward to inner handler)
// -----------------------------------------------------------------

void FInputMessageBridge::OnWindowResized(FGenericWindow* window, int32_t w, int32_t h)
{
	if (InnerHandler)
	{
		InnerHandler->OnWindowResized(window, w, h);
	}
}

void FInputMessageBridge::OnWindowFocusChanged(FGenericWindow* window, bool bHasFocus)
{
	if (InnerHandler)
	{
		InnerHandler->OnWindowFocusChanged(window, bHasFocus);
	}
}

bool FInputMessageBridge::OnWindowCloseRequested(FGenericWindow* window)
{
	if (InnerHandler)
	{
		return InnerHandler->OnWindowCloseRequested(window);
	}
	return true;
}

// -----------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------

FKey FInputMessageBridge::MouseButtonToKey(EMouseButton button)
{
	switch (button)
	{
	case EMouseButton::Left:   return EKeys::LeftMouseButton;
	case EMouseButton::Right:  return EKeys::RightMouseButton;
	case EMouseButton::Middle: return EKeys::MiddleMouseButton;
	default:                   return EKeys::None;
	}
}

} // namespace Enigma