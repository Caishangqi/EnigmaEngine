// Copyright EnigmaEngine. All Rights Reserved.

#include "GameFramework/GameObject.h"
#include "GameFramework/Scene.h"
#include "GameFramework/SceneManager.h"
#include "GameFramework/GameInstance.h"
#include "Engine/Engine.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameObject, Info, All);

namespace Enigma
{

// ---------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------

FGameObject::FGameObject(uint64_t id, const std::string& name, FScene* scene)
	: m_id(id)
	, m_name(name)
	, m_scene(scene)
{
	check(id != 0);
	m_transform.OnAttach(this);
	ENIGMA_LOG(LogGameObject, Verbose, "GameObject '{}' created (ID={})", m_name, m_id);
}

FGameObject::~FGameObject()
{
	// Detach components in reverse attachment order
	for (auto it = m_components.rbegin(); it != m_components.rend(); ++it)
	{
		(*it)->OnDetach();
	}
	m_components.clear();

	m_transform.OnDetach();
	ENIGMA_LOG(LogGameObject, Verbose, "GameObject '{}' destroyed (ID={})", m_name, m_id);
}

// ---------------------------------------------------------------
// Static Factory
// ---------------------------------------------------------------

FGameObject* FGameObject::Create(const std::string& name)
{
	checkf(GEngine, "GEngine is null -- engine not initialized");
	FGameInstance* gi = GEngine->GetGameInstance();
	checkf(gi, "No GameInstance available");
	FScene* scene = gi->GetActiveScene();
	checkf(scene, "No active scene -- call LoadScene() before Create()");
	return scene->CreateGameObject(name);
}

// ---------------------------------------------------------------
// Frame Update
// ---------------------------------------------------------------

void FGameObject::Update(float deltaTime)
{
	if (!m_bActive)
	{
		return;
	}

	for (auto& comp : m_components)
	{
		if (!comp->IsEnabled())
		{
			continue;
		}

		if (!comp->HasBegunPlay())
		{
			comp->BeginPlay();
		}

		comp->Update(deltaTime);
	}
}

} // namespace Enigma
