// Copyright EnigmaEngine. All Rights Reserved.

#include "ArcadeGameInstance.h"
#include "ArcadeMovementComponent.h"
#include "ArcadeBoundsClampComponent.h"
#include "AsciiRenderer/AsciiSpriteComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/GameObject.h"
#include "GameFramework/Scene.h"
#include "Modules/ModuleManager.h"
#include "RenderCore/AsciiRendererInterface.h"
#include "RenderCore/AsciiCell.h"
#include "Math/Color.h"
#include "Math/Vector.h"
#include "CoreGlobals.h"
#include "Logging/LogMacros.h"
#include "Logging/LogCategory.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>

DEFINE_LOG_CATEGORY_STATIC(LogArcade, Info, All);

void FArcadeGameInstance::Init()
{
	Enigma::FGameInstance::Init();

	// Configure accumulation for multi-key diagonal input
	m_moveAction.SetAccumulationBehavior(
		Enigma::EInputActionAccumulationBehavior::Cumulative);
	m_resizeAction.SetAccumulationBehavior(
		Enigma::EInputActionAccumulationBehavior::Cumulative);

	// Configure modifiers: negateX flips X only, swizzle maps X->Y
	m_negateX.bX = true;
	m_negateX.bY = false;
	m_negateX.bZ = false;
	m_swizzleYXZ.Order = Enigma::ESwizzleAxis::YXZ;

	// Create the game scene and player object
	Enigma::FScene* scene = LoadScene("ArcadeLevel");

	m_playerObj = scene->CreateGameObject("Player");
	m_playerObj->GetTransform().SetPosition(Enigma::FVector(20.0f, 20.0f, 0.0f));

	m_playerSprite = m_playerObj->AddComponent<Enigma::FAsciiSpriteComponent>();
	m_playerSprite->DisplayChar = '3';
	m_playerSprite->Width  = 5;
	m_playerSprite->Height = 5;
	m_playerSprite->Fg = Enigma::FColor::Green;
	m_playerSprite->Bg = Enigma::FColor::Black;

	// Add tick-driven components for movement and bounds clamping
	m_movementComp = m_playerObj->AddComponent<FArcadeMovementComponent>();
	m_playerObj->AddComponent<FArcadeBoundsClampComponent>();

	ENIGMA_LOG(LogArcade, Info, "ArcadeGameInstance initialized (Enhanced Input demo)");
}

void FArcadeGameInstance::SetupInput(Enigma::FInputSubsystem& InputSubsystem)
{
	m_inputSubsystem = &InputSubsystem;

	// ---------------------------------------------------------------
	// Move Context: WASD with Down trigger (continuous each frame)
	// Y-up convention: W=+Y (up), S=-Y (down), D=+X (right), A=-X (left)
	// ---------------------------------------------------------------
	// D -> +X
	m_moveContext.MapKey(&m_moveAction, Enigma::EKeys::D)
		.Triggers.push_back(&m_moveDownTrigger);
	// A -> -X
	{
		auto& m = m_moveContext.MapKey(&m_moveAction, Enigma::EKeys::A);
		m.Modifiers.push_back(&m_negateX);
		m.Triggers.push_back(&m_moveDownTrigger);
	}
	// W -> +Y (swizzle X->Y)
	{
		auto& m = m_moveContext.MapKey(&m_moveAction, Enigma::EKeys::W);
		m.Modifiers.push_back(&m_swizzleYXZ);
		m.Triggers.push_back(&m_moveDownTrigger);
	}
	// S -> -Y (negate then swizzle)
	{
		auto& m = m_moveContext.MapKey(&m_moveAction, Enigma::EKeys::S);
		m.Modifiers.push_back(&m_negateX);
		m.Modifiers.push_back(&m_swizzleYXZ);
		m.Triggers.push_back(&m_moveDownTrigger);
	}
	// TAB -> toggle context
	m_moveContext.MapKey(&m_toggleAction, Enigma::EKeys::Tab)
		.Triggers.push_back(&m_toggleTrigger);
	// ESC -> exit
	m_moveContext.MapKey(&m_exitAction, Enigma::EKeys::Escape)
		.Triggers.push_back(&m_exitTrigger);

	// ---------------------------------------------------------------
	// Resize Context: WASD with Pressed trigger (one-shot per press)
	// ---------------------------------------------------------------
	m_resizeContext.MapKey(&m_resizeAction, Enigma::EKeys::D)
		.Triggers.push_back(&m_resizePressedTrigger);
	{
		auto& m = m_resizeContext.MapKey(&m_resizeAction, Enigma::EKeys::A);
		m.Modifiers.push_back(&m_negateX);
		m.Triggers.push_back(&m_resizePressedTrigger);
	}
	{
		auto& m = m_resizeContext.MapKey(&m_resizeAction, Enigma::EKeys::W);
		m.Modifiers.push_back(&m_swizzleYXZ);
		m.Triggers.push_back(&m_resizePressedTrigger);
	}
	{
		auto& m = m_resizeContext.MapKey(&m_resizeAction, Enigma::EKeys::S);
		m.Modifiers.push_back(&m_negateX);
		m.Modifiers.push_back(&m_swizzleYXZ);
		m.Triggers.push_back(&m_resizePressedTrigger);
	}
	m_resizeContext.MapKey(&m_toggleAction, Enigma::EKeys::Tab)
		.Triggers.push_back(&m_toggleTrigger);
	m_resizeContext.MapKey(&m_exitAction, Enigma::EKeys::Escape)
		.Triggers.push_back(&m_exitTrigger);

	// ---------------------------------------------------------------
	// Bind action callbacks
	// ---------------------------------------------------------------

	// Move: set velocity on movement component (applied in component Update)
	Enigma::FInputActionCallback moveCb;
	moveCb.Bind([this](const Enigma::FInputActionInstance& inst)
	{
		Enigma::FVector v = inst.Value.Get<Enigma::FVector>();
		m_movementComp->VelX = v.X * m_movementComp->MoveSpeed;
		m_movementComp->VelY = v.Y * m_movementComp->MoveSpeed;
	});
	InputBindingHandles.push_back(InputSubsystem.BindAction(&m_moveAction,
		Enigma::ETriggerEvent::Triggered, std::move(moveCb)));

	// Resize: adjust width/height directly (discrete, one-shot)
	Enigma::FInputActionCallback resizeCb;
	resizeCb.Bind([this](const Enigma::FInputActionInstance& inst)
	{
		Enigma::FVector v = inst.Value.Get<Enigma::FVector>();
		m_playerSprite->Width  += static_cast<int32_t>(v.X);
		m_playerSprite->Height += static_cast<int32_t>(v.Y);
		m_playerSprite->Width  = std::max(m_playerSprite->Width, 1);
		m_playerSprite->Height = std::max(m_playerSprite->Height, 1);
	});
	InputBindingHandles.push_back(InputSubsystem.BindAction(&m_resizeAction,
		Enigma::ETriggerEvent::Triggered, std::move(resizeCb)));

	// Toggle: swap mapping contexts
	Enigma::FInputActionCallback toggleCb;
	toggleCb.Bind([this](const Enigma::FInputActionInstance&)
	{
		m_bMoveMode = !m_bMoveMode;
		if (m_bMoveMode)
		{
			m_inputSubsystem->RemoveMappingContext(&m_resizeContext);
			m_inputSubsystem->AddMappingContext(&m_moveContext, 0);
			m_playerSprite->Fg = Enigma::FColor::Green;
		}
		else
		{
			m_inputSubsystem->RemoveMappingContext(&m_moveContext);
			m_inputSubsystem->AddMappingContext(&m_resizeContext, 0);
			m_playerSprite->Fg = Enigma::FColor::Yellow;
		}
		ENIGMA_LOG(LogArcade, Info, "Mode: {}",
			m_bMoveMode ? "Move" : "Resize");
	});
	InputBindingHandles.push_back(InputSubsystem.BindAction(&m_toggleAction,
		Enigma::ETriggerEvent::Triggered, std::move(toggleCb)));

	// Exit: request engine shutdown
	Enigma::FInputActionCallback exitCb;
	exitCb.Bind([](const Enigma::FInputActionInstance&)
	{
		ENIGMA_LOG(LogArcade, Info, "ESC pressed, requesting exit");
		Enigma::RequestEngineExit("ESC pressed");
	});
	InputBindingHandles.push_back(InputSubsystem.BindAction(&m_exitAction,
		Enigma::ETriggerEvent::Triggered, std::move(exitCb)));

	// Start with Move context active
	InputSubsystem.AddMappingContext(&m_moveContext, 0);

	ENIGMA_LOG(LogArcade, Info, "Input setup complete (Move mode)");
}

void FArcadeGameInstance::Update(float deltaTime)
{
	// Movement and bounds clamping are now handled by tick-driven components:
	//   FArcadeMovementComponent (TG_Update) -- velocity application
	//   FArcadeBoundsClampComponent (TG_PostUpdate) -- boundary clamping
	Enigma::FGameInstance::Update(deltaTime);
}

void FArcadeGameInstance::Render()
{
	// Drive scene-based rendering (FAsciiSpriteComponent draws the player).
	Enigma::FGameInstance::Render();

	// Status text overlay at top of screen (Y-up: bufH-1 is the top row).
	auto& renderer = Enigma::FModuleManager::Get()
		.GetModuleChecked<Enigma::IAsciiRendererModule>("Renderer");
	int32_t topY = renderer.GetFrameBufferHeight() - 1;

	const Enigma::FVector pos = m_playerObj->GetTransform().GetPosition();
	int32_t rx = static_cast<int32_t>(std::round(pos.X));
	int32_t ry = static_cast<int32_t>(std::round(pos.Y));
	const char* mode = m_bMoveMode ? "MOVE" : "RESIZE";
	std::string status = std::format("[{}] pos({},{}) size({}x{}) TAB=toggle ESC=quit",
		mode, rx, ry, m_playerSprite->Width, m_playerSprite->Height);
	renderer.DrawText(0, topY, 1, status.c_str(),
		Enigma::FColor::White, Enigma::FColor::Blue);
}

void FArcadeGameInstance::Shutdown()
{
	if (m_inputSubsystem)
	{
		for (const Enigma::FInputBindingHandle& Handle : InputBindingHandles)
		{
			m_inputSubsystem->UnbindAction(Handle);
		}
		InputBindingHandles.clear();
		m_inputSubsystem->RemoveMappingContext(&m_moveContext);
		m_inputSubsystem->RemoveMappingContext(&m_resizeContext);
		m_inputSubsystem = nullptr;
	}

	ENIGMA_LOG(LogArcade, Info, "ArcadeGameInstance shutdown");
	Enigma::FGameInstance::Shutdown();
}
