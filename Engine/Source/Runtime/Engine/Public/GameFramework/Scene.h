// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file Scene.h
/// @brief Flat scene container managing FGameObject lifecycle and render registry.

#include "EngineAPI.generated.h"
#include "GameFramework/GameObject.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Enigma
{

class FRenderComponent;

/// @brief Flat scene container that owns and manages FGameObjects.
///
/// Responsibilities:
///   - Create/destroy game objects with unique IDs
///   - Drive per-frame Update on all active objects
///   - Maintain a render component registry for decoupled render traversal
///   - Deferred destruction: objects marked for destroy are cleaned up at frame end
///
/// Unity equivalent: Scene
/// UE equivalent: UWorld (simplified, flat, no levels)
class ENGINE_API FScene
{
public:
	explicit FScene(const std::string& name);
	~FScene();

	// Non-copyable, non-movable
	FScene(const FScene&) = delete;
	FScene& operator=(const FScene&) = delete;
	FScene(FScene&&) = delete;
	FScene& operator=(FScene&&) = delete;

	// ----- Object Management -----

	/// Create a new FGameObject in this scene.
	/// Assigns a unique monotonic ID and sets the object's scene pointer.
	FGameObject* CreateGameObject(const std::string& name);

	/// Mark a game object for deferred destruction (processed at frame end).
	void DestroyGameObject(uint64_t id);

	/// Find the first game object with the given name.
	/// @return Pointer to the object, or nullptr if not found.
	[[nodiscard]] FGameObject* FindGameObject(std::string_view name) const;

	/// Find a game object by its unique ID.
	/// @return Pointer to the object, or nullptr if not found.
	[[nodiscard]] FGameObject* FindGameObjectByID(uint64_t id) const;

	/// Get a read-only view of all game objects.
	[[nodiscard]] std::span<const std::unique_ptr<FGameObject>> GetAllGameObjects() const;

	// ----- Frame Loop -----

	/// Drive one frame: BeginPlay (new components) -> Update -> processPendingDestroys.
	void Tick(float deltaTime);

	/// Render all registered render components (skip inactive owners).
	void RenderScene();

	// ----- Render Registry -----

	/// Register a render component (called by FRenderComponent::OnAttach).
	void RegisterRenderComponent(FRenderComponent* comp);

	/// Unregister a render component (called by FRenderComponent::OnDetach).
	void UnregisterRenderComponent(FRenderComponent* comp);

	// ----- Accessors -----

	[[nodiscard]] std::string_view GetName() const noexcept { return m_name; }

private:
	/// Clean up objects marked for deferred destruction.
	void processPendingDestroys();

	std::string m_name;
	std::vector<std::unique_ptr<FGameObject>> m_gameObjects;
	std::vector<FRenderComponent*> m_renderComponents; // non-owning registry
	std::vector<uint64_t> m_pendingDestroys;
	uint64_t m_nextObjectID = 1;
};

} // namespace Enigma
