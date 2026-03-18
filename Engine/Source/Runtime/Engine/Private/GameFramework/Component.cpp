// Copyright EnigmaEngine. All Rights Reserved.

#include "GameFramework/Component.h"
#include "TickSystem/TickFunction.h"
#include "TickSystem/TickTaskManager.h"
#include "Engine/Engine.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogComponent, Info, All);

namespace Enigma
{

FComponent::FComponent() = default;
FComponent::~FComponent() = default;

void FComponent::OnAttach(FGameObject* owner)
{
	m_owner = owner;
	ENIGMA_LOG(LogComponent, Verbose, "Component '{}' attached", *GetName());

	// Create and register tick function if this component wants to tick
	if (bCanEverTick)
	{
		m_tickFunction = std::make_unique<FComponentTickFunction>();
		m_tickFunction->Target              = this;
		m_tickFunction->TickGroup           = TickGroup;
		m_tickFunction->TickInterval        = TickInterval;
		m_tickFunction->bStartWithTickEnabled = bStartWithTickEnabled;
		m_tickFunction->bCanEverTick        = true;

		if (GEngine)
		{
			auto* tickManager = GEngine->GetSubsystem<FTickTaskManager>();
			if (tickManager)
			{
				m_tickFunction->RegisterTickFunction(*tickManager);
			}
		}
	}
}

void FComponent::BeginPlay()
{
	if (m_bBegunPlay)
	{
		return;
	}
	m_bBegunPlay = true;
	ENIGMA_LOG(LogComponent, Verbose, "Component '{}' BeginPlay", *GetName());
}

void FComponent::Update(float /*deltaTime*/)
{
	// Default: no-op. Derived classes override for game logic.
}

void FComponent::SetEnabled(bool bEnabled)
{
	m_bEnabled = bEnabled;

	if (m_tickFunction)
	{
		m_tickFunction->SetTickFunctionEnable(bEnabled);
	}
}

void FComponent::OnDetach()
{
	// Unregister tick function before clearing owner
	if (m_tickFunction)
	{
		m_tickFunction->UnregisterTickFunction();
		m_tickFunction.reset();
	}

	ENIGMA_LOG(LogComponent, Verbose, "Component '{}' detached", *GetName());
	m_owner = nullptr;
	m_bBegunPlay = false;
}

} // namespace Enigma
