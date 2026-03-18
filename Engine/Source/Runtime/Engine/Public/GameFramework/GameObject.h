// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file GameObject.h
/// @brief Game entity container -- pure composition, no inheritance.

#include "EngineAPI.generated.h"
#include "GameFramework/Component.h"
#include "GameFramework/TransformComponent.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Name.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Enigma
{

class FScene;

/// @brief Game entity (final, not inheritable).
///
/// All behavior differences are achieved through component composition.
/// FGameObject is created exclusively by FScene (friend) or via the
/// static Create() factory which delegates to the active scene.
///
/// Unity equivalent: GameObject (sealed)
/// UE equivalent: AActor (simplified, no hierarchy)
class ENGINE_API FGameObject final
{
	friend class FScene; // FScene creates and manages FGameObject instances

public:
	~FGameObject();

	// Non-copyable, non-movable
	FGameObject(const FGameObject&) = delete;
	FGameObject& operator=(const FGameObject&) = delete;
	FGameObject(FGameObject&&) = delete;
	FGameObject& operator=(FGameObject&&) = delete;

	// ----- Static Factory -----

	/// Create a new FGameObject in the currently active scene.
	/// Unity-style convenience: auto-adds to active scene via GEngine.
	/// @pre An active scene must exist (asserts otherwise).
	static FGameObject* Create(const std::string& name);

	// ----- Component Management -----

	/// Create and attach a component of type T.
	/// Calls OnAttach() immediately. Multiple components of the same type are allowed.
	/// @return Raw pointer to the newly created component (owned by this FGameObject).
	template <typename T, typename... Args>
	T* AddComponent(Args&&... args);

	/// Find the first component of type T.
	/// @return Pointer to the component, or nullptr if not found.
	template <typename T>
	T* GetComponent() const;

	/// Find all components of type T.
	template <typename T>
	std::vector<T*> GetComponents() const;

	/// Remove and destroy the first component of type T.
	/// Cannot remove the built-in FTransformComponent.
	/// @return true if a component was removed, false if not found.
	template <typename T>
	bool RemoveComponent();

	// ----- Built-in Transform -----

	FTransformComponent& GetTransform() { return m_transform; }
	const FTransformComponent& GetTransform() const { return m_transform; }

	// ----- Identity -----

	[[nodiscard]] uint64_t GetID() const noexcept { return m_id; }
	[[nodiscard]] std::string_view GetName() const noexcept { return m_name; }

	// ----- Scene -----

	[[nodiscard]] FScene* GetScene() const noexcept { return m_scene; }

	// ----- Active State -----

	[[nodiscard]] bool IsActive() const noexcept { return m_bActive; }
	void SetActive(bool bActive) noexcept { m_bActive = bActive; }

	// ----- Frame Update (called by FScene) -----

	/// No-op. Component tick dispatch is handled by FTickTaskManager.
	void Update(float deltaTime);

private:
	/// Private constructor -- only FScene can create instances.
	FGameObject(uint64_t id, const std::string& name, FScene* scene);

	/// Dispatch BeginPlay on a component if the scene has already begun play.
	/// Called from AddComponent after OnAttach.
	void dispatchBeginPlayIfReady(FComponent* comp);

	uint64_t m_id;
	std::string m_name;
	bool m_bActive = true;
	FScene* m_scene = nullptr;
	FTransformComponent m_transform;
	std::vector<std::unique_ptr<FComponent>> m_components;
};

// ---------------------------------------------------------------
// Template implementations
// ---------------------------------------------------------------

template <typename T, typename... Args>
T* FGameObject::AddComponent(Args&&... args)
{
	static_assert(std::is_base_of_v<FComponent, T>, "T must derive from FComponent");

	auto component = std::make_unique<T>(std::forward<Args>(args)...);
	T* raw = component.get();
	raw->OnAttach(this);
	m_components.push_back(std::move(component));
	dispatchBeginPlayIfReady(raw);
	return raw;
}

template <typename T>
T* FGameObject::GetComponent() const
{
	static_assert(std::is_base_of_v<FComponent, T>, "T must derive from FComponent");

	const FName targetName = T::GetStaticName();
	for (const auto& comp : m_components)
	{
		if (comp->GetName() == targetName)
		{
			return static_cast<T*>(comp.get());
		}
	}
	return nullptr;
}

template <typename T>
std::vector<T*> FGameObject::GetComponents() const
{
	static_assert(std::is_base_of_v<FComponent, T>, "T must derive from FComponent");

	std::vector<T*> result;
	const FName targetName = T::GetStaticName();
	for (const auto& comp : m_components)
	{
		if (comp->GetName() == targetName)
		{
			result.push_back(static_cast<T*>(comp.get()));
		}
	}
	return result;
}

template <typename T>
bool FGameObject::RemoveComponent()
{
	static_assert(std::is_base_of_v<FComponent, T>, "T must derive from FComponent");

	const FName targetName = T::GetStaticName();

	checkf(targetName != FTransformComponent::GetStaticName(),
		"Cannot remove built-in TransformComponent");

	for (auto it = m_components.begin(); it != m_components.end(); ++it)
	{
		if ((*it)->GetName() == targetName)
		{
			(*it)->OnDetach();
			m_components.erase(it);
			return true;
		}
	}
	return false;
}

} // namespace Enigma
