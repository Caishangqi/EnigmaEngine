// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file InputMessageBridge.h
/// @brief Bridges platform input events to FInputSubsystem key states.

#include "InputKeys.h"  // for ENHANCEDINPUT_API
#include "GenericPlatform/GenericApplicationMessageHandler.h"

namespace Enigma
{

class FInputSubsystem;

/// @brief Bridges platform input events to FInputSubsystem key states.
///
/// Installed as a decorator around the existing message handler.
/// UE equivalent: FEnhancedInputWorldProcessor
class ENHANCEDINPUT_API FInputMessageBridge : public FGenericApplicationMessageHandler
{
public:
	explicit FInputMessageBridge(
		FInputSubsystem* inputSubsystem,
		FGenericApplicationMessageHandler* innerHandler = nullptr);

	// Keyboard -> translate keyCode to FKey, update subsystem state
	bool OnKeyDown(int32_t keyCode, uint32_t charCode, bool bIsRepeat) override;
	bool OnKeyUp(int32_t keyCode, uint32_t charCode, bool bIsRepeat) override;

	// Mouse buttons -> translate EMouseButton to FKey
	bool OnMouseDown(EMouseButton button, float cursorX, float cursorY) override;
	bool OnMouseUp(EMouseButton button, float cursorX, float cursorY) override;

	// Mouse axes -> update axis values
	bool OnMouseMove(float cursorX, float cursorY) override;
	bool OnMouseWheel(float delta, float cursorX, float cursorY) override;

	// Window events -> forward to inner handler
	void OnWindowResized(FGenericWindow* window, int32_t w, int32_t h) override;
	void OnWindowFocusChanged(FGenericWindow* window, bool bHasFocus) override;
	bool OnWindowCloseRequested(FGenericWindow* window) override;

private:
	FInputSubsystem* InputSubsystem = nullptr;
	FGenericApplicationMessageHandler* InnerHandler = nullptr;

	static FKey MouseButtonToKey(EMouseButton button);
};

} // namespace Enigma
