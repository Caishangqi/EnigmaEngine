// Copyright EnigmaEngine. All Rights Reserved.

#include "GameFramework/Component.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogComponent, Info, All);

namespace Enigma
{

void FComponent::OnAttach(FGameObject* owner)
{
	m_owner = owner;
	ENIGMA_LOG(LogComponent, Verbose, "Component '{}' attached", *GetName());
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

void FComponent::OnDetach()
{
	ENIGMA_LOG(LogComponent, Verbose, "Component '{}' detached", *GetName());
	m_owner = nullptr;
	m_bBegunPlay = false;
}

} // namespace Enigma
