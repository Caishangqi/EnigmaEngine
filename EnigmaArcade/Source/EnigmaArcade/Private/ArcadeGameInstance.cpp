// Copyright EnigmaEngine. All Rights Reserved.

#include "ArcadeGameInstance.h"
#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"
#include "RenderCore/AsciiRendererInterface.h"
#include "RenderCore/AsciiCell.h"
#include "Math/Color.h"
#include "Math/Vector.h"
#include "CoreGlobals.h"
#include "Logging/LogMacros.h"
#include "Logging/LogCategory.h"

#include <algorithm>
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

	// Initial position (Y-up: bottom-left origin)
	m_player.PosX = 5.0f;
	m_player.PosY = 5.0f;

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

	// Move: set velocity (applied in Update via FAsciiGameObject)
	Enigma::FInputActionCallback moveCb;
	moveCb.Bind([this](const Enigma::FInputActionInstance& inst)
	{
		Enigma::FVector v = inst.Value.Get<Enigma::FVector>();
		m_player.VelX = v.X * m_moveSpeed;
		m_player.VelY = v.Y * m_moveSpeed;
	});
	InputSubsystem.BindAction(&m_moveAction,
		Enigma::ETriggerEvent::Triggered, std::move(moveCb));

	// Resize: adjust width/height directly (discrete, one-shot)
	Enigma::FInputActionCallback resizeCb;
	resizeCb.Bind([this](const Enigma::FInputActionInstance& inst)
	{
		Enigma::FVector v = inst.Value.Get<Enigma::FVector>();
		m_player.Width  += static_cast<int32_t>(v.X);
		m_player.Height += static_cast<int32_t>(v.Y);
		m_player.Width  = std::max(m_player.Width, 1);
		m_player.Height = std::max(m_player.Height, 1);
	});
	InputSubsystem.BindAction(&m_resizeAction,
		Enigma::ETriggerEvent::Triggered, std::move(resizeCb));

	// Toggle: swap mapping contexts
	Enigma::FInputActionCallback toggleCb;
	toggleCb.Bind([this](const Enigma::FInputActionInstance&)
	{
		m_bMoveMode = !m_bMoveMode;
		if (m_bMoveMode)
		{
			m_inputSubsystem->RemoveMappingContext(&m_resizeContext);
			m_inputSubsystem->AddMappingContext(&m_moveContext, 0);
			m_player.Fg = Enigma::FColor::Green;
		}
		else
		{
			m_inputSubsystem->RemoveMappingContext(&m_moveContext);
			m_inputSubsystem->AddMappingContext(&m_resizeContext, 0);
			m_player.Fg = Enigma::FColor::Yellow;
		}
		ENIGMA_LOG(LogArcade, Info, "Mode: {}",
			m_bMoveMode ? "Move" : "Resize");
	});
	InputSubsystem.BindAction(&m_toggleAction,
		Enigma::ETriggerEvent::Triggered, std::move(toggleCb));

	// Exit: request engine shutdown
	Enigma::FInputActionCallback exitCb;
	exitCb.Bind([](const Enigma::FInputActionInstance&)
	{
		ENIGMA_LOG(LogArcade, Info, "ESC pressed, requesting exit");
		Enigma::RequestEngineExit("ESC pressed");
	});
	InputSubsystem.BindAction(&m_exitAction,
		Enigma::ETriggerEvent::Triggered, std::move(exitCb));

	// Start with Move context active
	InputSubsystem.AddMappingContext(&m_moveContext, 0);

	ENIGMA_LOG(LogArcade, Info, "Input setup complete (Move mode)");
}

void FArcadeGameInstance::Update(float deltaTime)
{
	Enigma::FGameInstance::Update(deltaTime);

	// Apply velocity and clamp to buffer bounds
	m_player.Update(deltaTime);

	auto& renderer = Enigma::FModuleManager::Get()
		.GetModuleChecked<Enigma::IAsciiRendererModule>("Renderer");
	m_player.ClampToBounds(renderer.GetFrameBufferWidth(),
	                       renderer.GetFrameBufferHeight());
}

void FArcadeGameInstance::Render()
{
	auto& renderer = Enigma::FModuleManager::Get()
		.GetModuleChecked<Enigma::IAsciiRendererModule>("Renderer");

	// Draw the player object
	m_player.Render(renderer, 0);

	// Status text at top of screen (Y-up: bufH-1 is the top row)
	int32_t topY = renderer.GetFrameBufferHeight() - 1;
	const char* mode = m_bMoveMode ? "MOVE" : "RESIZE";
	int32_t rx = static_cast<int32_t>(std::round(m_player.PosX));
	int32_t ry = static_cast<int32_t>(std::round(m_player.PosY));
	std::string status = std::format("[{}] pos({},{}) size({}x{}) TAB=toggle ESC=quit",
		mode, rx, ry, m_player.Width, m_player.Height);
	renderer.DrawText(0, topY, 1, status.c_str(),
		Enigma::FColor::White, Enigma::FColor::Blue);
}

void FArcadeGameInstance::Shutdown()
{
	ENIGMA_LOG(LogArcade, Info, "ArcadeGameInstance shutdown");
	Enigma::FGameInstance::Shutdown();
}