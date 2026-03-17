// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "EngineAPI.generated.h"
#include "GameFramework/SceneManager.h"

#include <cstdint>
#include <string>

namespace Enigma
{

class FInputSubsystem; // forward declaration for SetupInput
class FScene;
class FGameObject;

// ---------------------------------------------------------------
// FGameInstance -- user-programmable game instance base class (REQ-013)
//
// Owns the per-frame game loop: BeginFrame -> Update -> Render -> EndFrame.
// Users derive from this class to implement game-specific logic.
// FGameEngine creates and owns the instance via std::unique_ptr.
//
// Lifecycle:
//   1. Created by FGameEngine::CreateGameInstance() during Init
//   2. Init() called by FGameEngine::Start()
//   3. Frame loop called each tick by FGameEngine::Tick():
//        BeginFrame() -> Update(dt) -> Render() -> EndFrame()
//   4. Shutdown() called by FGameEngine::Shutdown()
//
// DeltaTime: stored in Update(), accessible via GetDeltaTime()
// FrameCount: incremented at the start of each frame (BeginFrame)
//
// Scene Management:
//   Base Update() drives FSceneManager::Tick() (scene transitions + object updates).
//   Base Render() drives FSceneManager::RenderScene().
//   If user overrides Update/Render without calling base, scene system is bypassed.
// ---------------------------------------------------------------
class ENGINE_API FGameInstance
{
public:
    virtual ~FGameInstance() = default;

    // ----- Lifecycle -----

    /// Called once after creation, before the first frame.
    virtual void Init();

    /// Called during engine shutdown, after the last frame.
    virtual void Shutdown();

    /// Called during Init() after input subsystem is available.
    /// Override to create actions, mapping contexts, and bind callbacks.
    ///
    /// UE equivalent: APawn::SetupPlayerInputComponent(UInputComponent*)
    /// UE binds on Pawn level; we bind on GameInstance level (no Pawn/PlayerController yet).
    virtual void SetupInput(FInputSubsystem& InputSubsystem) {}

    // ----- Per-frame loop (called in order by FGameEngine::Tick) -----

    /// Called at the start of each frame. Increments FrameCount.
    virtual void BeginFrame();

    /// Called with the current frame's delta time.
    /// Base implementation drives FSceneManager::Tick().
    virtual void Update(float deltaTime);

    /// Called after Update.
    /// Base implementation drives FSceneManager::RenderScene().
    virtual void Render();

    /// Called at the end of each frame. Override for cleanup/finalization.
    virtual void EndFrame();

    // ----- Scene Management -----

    /// Get the scene manager.
    FSceneManager& GetSceneManager() { return m_sceneManager; }
    const FSceneManager& GetSceneManager() const { return m_sceneManager; }

    /// Convenience: load a new scene (delegates to SceneManager).
    FScene* LoadScene(const std::string& name);

    /// Convenience: get the currently active scene.
    FScene* GetActiveScene() const;

    /// Convenience: create a GameObject in the active scene.
    FGameObject* CreateGameObject(const std::string& name);

    // ----- Accessors -----

    float    GetDeltaTime()  const { return DeltaTime; }
    uint64_t GetFrameCount() const { return FrameCount; }

protected:
    float    DeltaTime  = 0.0f;
    uint64_t FrameCount = 0;
    FSceneManager m_sceneManager;
};

} // namespace Enigma
