// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file SceneManager.h
/// @brief Manages scene lifecycle and safe transitions between scenes.

#include "EngineAPI.generated.h"
#include "GameFramework/Scene.h"

#include <memory>
#include <string>
#include <string_view>

namespace Enigma
{

/// @brief Manages the active scene and handles safe scene transitions.
///
/// Scene switching uses double-buffering: LoadScene() stores the new scene
/// as pending, and the actual swap happens at the start of the next Tick()
/// (frame boundary), ensuring no mid-tick disruption.
///
/// Single active scene model -- no additive scene loading.
///
/// Unity equivalent: SceneManager
/// UE equivalent: UWorld/UGameplayStatics::OpenLevel (simplified)
class ENGINE_API FSceneManager
{
public:
	FSceneManager() = default;
	~FSceneManager() = default;

	// Non-copyable, non-movable
	FSceneManager(const FSceneManager&) = delete;
	FSceneManager& operator=(const FSceneManager&) = delete;
	FSceneManager(FSceneManager&&) = delete;
	FSceneManager& operator=(FSceneManager&&) = delete;

	// ----- Scene Loading -----

	/// Request loading a new scene. The transition happens at the next Tick() entry.
	/// The previous scene is fully destroyed when the new scene becomes active.
	/// @return Pointer to the newly created (pending) scene.
	FScene* LoadScene(const std::string& name);

	// ----- Accessors -----

	/// Get the currently active scene, or nullptr if none.
	[[nodiscard]] FScene* GetActiveScene() const noexcept { return m_activeScene.get(); }

	/// Get the name of the active scene, or empty if none.
	[[nodiscard]] std::string_view GetActiveSceneName() const noexcept;

	// ----- Frame Loop (called by FGameInstance) -----

	/// Process pending scene transition, then tick the active scene.
	void Tick(float deltaTime);

	/// Render the active scene.
	void RenderScene();

private:
	std::unique_ptr<FScene> m_activeScene;
	std::unique_ptr<FScene> m_pendingScene;
	bool m_bSceneTransitionRequested = false;
};

} // namespace Enigma
