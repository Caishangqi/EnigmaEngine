// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "EngineAPI.generated.h"

#include <cstdint>

namespace Enigma
{

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

    // ----- Per-frame loop (called in order by FGameEngine::Tick) -----

    /// Called at the start of each frame. Increments FrameCount.
    virtual void BeginFrame();

    /// Called with the current frame's delta time. Override for game logic.
    virtual void Update(float deltaTime);

    /// Called after Update. Override for rendering logic.
    virtual void Render();

    /// Called at the end of each frame. Override for cleanup/finalization.
    virtual void EndFrame();

    // ----- Accessors -----

    float    GetDeltaTime()  const { return DeltaTime; }
    uint64_t GetFrameCount() const { return FrameCount; }

protected:
    float    DeltaTime  = 0.0f;
    uint64_t FrameCount = 0;
};

} // namespace Enigma
