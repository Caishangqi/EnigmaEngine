// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "EnigmaArcadeAPI.generated.h"
#include "GameFramework/GameInstance.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "InputSubsystem.h"

namespace Enigma { class FGameObject; class FAsciiSpriteComponent; }
class FArcadeMovementComponent;

/// Game instance for EnigmaArcade.
/// Demonstrates the Enhanced Input system with two mapping contexts:
///   - Move mode (WASD continuous, Down trigger, velocity-based)
///   - Resize mode (WASD one-shot, Pressed trigger, discrete)
/// TAB toggles between modes, ESC exits.
/// Uses FGameObject + FAsciiSpriteComponent (component-based architecture).
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

	// --- Player (scene-managed) ---
	Enigma::FGameObject* m_playerObj = nullptr;
	Enigma::FAsciiSpriteComponent* m_playerSprite = nullptr;
	FArcadeMovementComponent* m_movementComp = nullptr;

	// --- State ---
	bool m_bMoveMode = true;
	Enigma::FInputSubsystem* m_inputSubsystem = nullptr;
};
