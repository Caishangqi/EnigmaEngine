// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "EnigmaArcadeAPI.generated.h"
#include "GameFramework/GameInstance.h"
#include "AsciiGameObject.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "InputSubsystem.h"

/// Game instance for EnigmaArcade.
/// Demonstrates the Enhanced Input system with two mapping contexts:
///   - Move mode (WASD continuous, Down trigger, velocity-based)
///   - Resize mode (WASD one-shot, Pressed trigger, discrete)
/// TAB toggles between modes, ESC exits.
/// Renders a movable/resizable '@' rectangle via FAsciiGameObject.
class ENIGMAARCADE_API FArcadeGameInstance : public Enigma::FGameInstance
{
public:
	void Init() override;
	void SetupInput(Enigma::FInputSubsystem& InputSubsystem) override;
	void Update(float deltaTime) override;
	void Render() override;
	void Shutdown() override;

private:
	// --- Input Actions (owned) ---
	Enigma::FInputAction m_moveAction{"Move", Enigma::EInputActionValueType::Axis2D};
	Enigma::FInputAction m_resizeAction{"Resize", Enigma::EInputActionValueType::Axis2D};
	Enigma::FInputAction m_toggleAction{"ToggleContext"};
	Enigma::FInputAction m_exitAction{"Exit"};

	// --- Mapping Contexts (owned) ---
	Enigma::FInputMappingContext m_moveContext{"MoveContext"};
	Enigma::FInputMappingContext m_resizeContext{"ResizeContext"};

	// --- Modifiers (owned, referenced by mappings) ---
	Enigma::FInputModifierNegate m_negateX;
	Enigma::FInputModifierSwizzleAxis m_swizzleYXZ;

	// --- Triggers (owned, one per action to avoid shared state) ---
	Enigma::FInputTriggerDown m_moveDownTrigger;
	Enigma::FInputTriggerPressed m_resizePressedTrigger;
	Enigma::FInputTriggerPressed m_toggleTrigger;
	Enigma::FInputTriggerPressed m_exitTrigger;

	// --- Game object ---
	Enigma::FAsciiGameObject m_player{'@', 2, 2, Enigma::FColor::Green, Enigma::FColor::Black};
	float m_moveSpeed = 15.0f; // cells per second

	// --- State ---
	bool m_bMoveMode = true;
	Enigma::FInputSubsystem* m_inputSubsystem = nullptr;
};
