// Copyright EnigmaEngine. All Rights Reserved.

#include "GameFramework/RenderComponent.h"
#include "GameFramework/GameObject.h"
#include "GameFramework/Scene.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogRenderComponent, Info, All);

namespace Enigma
{

void FRenderComponent::OnAttach(FGameObject* owner)
{
	FComponent::OnAttach(owner);

	FScene* scene = owner->GetScene();
	if (scene)
	{
		scene->RegisterRenderComponent(this);
		ENIGMA_LOG(LogRenderComponent, Verbose, "RenderComponent '{}' registered with scene", *GetName());
	}
}

void FRenderComponent::OnDetach()
{
	FScene* scene = GetOwner() ? GetOwner()->GetScene() : nullptr;
	if (scene)
	{
		scene->UnregisterRenderComponent(this);
		ENIGMA_LOG(LogRenderComponent, Verbose, "RenderComponent '{}' unregistered from scene", *GetName());
	}

	FComponent::OnDetach();
}

} // namespace Enigma
