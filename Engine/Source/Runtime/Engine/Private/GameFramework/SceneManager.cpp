// Copyright EnigmaEngine. All Rights Reserved.

#include "GameFramework/SceneManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogSceneManager, Info, All);

namespace Enigma
{

FScene* FSceneManager::LoadScene(const std::string& name)
{
	m_pendingScene = std::make_unique<FScene>(name);
	m_bSceneTransitionRequested = true;
	ENIGMA_LOG(LogSceneManager, Info, "Scene '{}' requested -- transition at next frame", name);
	return m_pendingScene.get();
}

std::string_view FSceneManager::GetActiveSceneName() const noexcept
{
	if (m_activeScene)
	{
		return m_activeScene->GetName();
	}
	return {};
}

void FSceneManager::Tick(float deltaTime)
{
	// Process pending scene transition at frame boundary
	if (m_bSceneTransitionRequested)
	{
		if (m_activeScene)
		{
			ENIGMA_LOG(LogSceneManager, Info, "Destroying previous scene '{}'", m_activeScene->GetName());
		}

		m_activeScene = std::move(m_pendingScene);
		m_bSceneTransitionRequested = false;

		ENIGMA_LOG(LogSceneManager, Info, "Scene '{}' is now active", m_activeScene->GetName());

		// Dispatch BeginPlay on the newly active scene (UE5 pattern).
		m_activeScene->BeginPlay();
	}

	// Tick the active scene
	if (m_activeScene)
	{
		m_activeScene->Tick(deltaTime);
	}
}

void FSceneManager::RenderScene()
{
	if (m_activeScene)
	{
		m_activeScene->RenderScene();
	}
}

} // namespace Enigma
