// Copyright EnigmaEngine. All Rights Reserved.

#include "GameFramework/Scene.h"
#include "GameFramework/RenderComponent.h"
#include "Misc/AssertionMacros.h"
#include "Logging/LogMacros.h"

#include <algorithm>

DEFINE_LOG_CATEGORY_STATIC(LogScene, Info, All);

namespace Enigma
{

FScene::FScene(const std::string& name)
	: m_name(name)
{
	ENIGMA_LOG(LogScene, Info, "Scene '{}' created", m_name);
}

FScene::~FScene()
{
	// Destroy all objects (triggers component OnDetach in reverse order via ~FGameObject)
	m_gameObjects.clear();
	m_renderComponents.clear();
	m_pendingDestroys.clear();
	ENIGMA_LOG(LogScene, Info, "Scene '{}' destroyed", m_name);
}

// ---------------------------------------------------------------
// Object Management
// ---------------------------------------------------------------

FGameObject* FScene::CreateGameObject(const std::string& name)
{
	uint64_t id = m_nextObjectID++;
	auto obj = std::unique_ptr<FGameObject>(new FGameObject(id, name, this));
	FGameObject* raw = obj.get();
	m_gameObjects.push_back(std::move(obj));
	ENIGMA_LOG(LogScene, Verbose, "Scene '{}': created GameObject '{}' (ID={})", m_name, name, id);
	return raw;
}

void FScene::DestroyGameObject(uint64_t id)
{
	if (!ensure(FindGameObjectByID(id) != nullptr))
	{
		ENIGMA_LOG(LogScene, Warning, "Scene '{}': DestroyGameObject called with invalid ID {}", m_name, id);
		return;
	}
	m_pendingDestroys.push_back(id);
}

FGameObject* FScene::FindGameObject(std::string_view name) const
{
	for (const auto& obj : m_gameObjects)
	{
		if (obj->GetName() == name)
		{
			return obj.get();
		}
	}
	return nullptr;
}

FGameObject* FScene::FindGameObjectByID(uint64_t id) const
{
	for (const auto& obj : m_gameObjects)
	{
		if (obj->GetID() == id)
		{
			return obj.get();
		}
	}
	return nullptr;
}

std::span<const std::unique_ptr<FGameObject>> FScene::GetAllGameObjects() const
{
	return m_gameObjects;
}

// ---------------------------------------------------------------
// Frame Loop
// ---------------------------------------------------------------

void FScene::Tick(float deltaTime)
{
	// Update all active game objects (BeginPlay + Update handled inside FGameObject::Update)
	for (auto& obj : m_gameObjects)
	{
		obj->Update(deltaTime);
	}

	// Clean up destroyed objects at frame end
	processPendingDestroys();
}

void FScene::RenderScene()
{
	for (auto* comp : m_renderComponents)
	{
		if (comp->GetOwner() && comp->GetOwner()->IsActive() && comp->IsEnabled())
		{
			comp->Render();
		}
	}
}

// ---------------------------------------------------------------
// Render Registry
// ---------------------------------------------------------------

void FScene::RegisterRenderComponent(FRenderComponent* comp)
{
	check(comp);
	m_renderComponents.push_back(comp);
}

void FScene::UnregisterRenderComponent(FRenderComponent* comp)
{
	auto it = std::find(m_renderComponents.begin(), m_renderComponents.end(), comp);
	if (it != m_renderComponents.end())
	{
		m_renderComponents.erase(it);
	}
}

// ---------------------------------------------------------------
// Deferred Destruction
// ---------------------------------------------------------------

void FScene::processPendingDestroys()
{
	if (m_pendingDestroys.empty())
	{
		return;
	}

	for (uint64_t id : m_pendingDestroys)
	{
		auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
			[id](const std::unique_ptr<FGameObject>& obj)
			{
				return obj->GetID() == id;
			});

		if (it != m_gameObjects.end())
		{
			ENIGMA_LOG(LogScene, Verbose, "Scene '{}': destroying GameObject '{}' (ID={})",
				m_name, (*it)->GetName(), id);
			m_gameObjects.erase(it);
		}
	}

	m_pendingDestroys.clear();
}

} // namespace Enigma
