// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "EngineAPI.generated.h"

#include <cstdint>

namespace Enigma
{

class FEngineLoop;
class FGameInstance;

// ---------------------------------------------------------------
// FEngine -- base class for the global engine instance (REQ-019)
//
// Provides the core engine lifecycle: Init, Start, Tick, Shutdown.
// FGameEngine (or editor engine) derives from this.
// A single global instance is accessible via GEngine.
// ---------------------------------------------------------------
class ENGINE_API FEngine
{
public:
    virtual ~FEngine() = default;

    /// Called by FEngineLoop::Init after creation.
    virtual void Init(FEngineLoop* engineLoop);

    /// Called after Init and PostEngineInit module loading.
    virtual void Start();

    /// Called every frame by FEngineLoop::Tick.
    virtual void Tick(float deltaTime);

    /// Called by FEngineLoop::Exit before destruction.
    virtual void Shutdown();

    /// Returns the game instance, or nullptr if not a game engine.
    virtual FGameInstance* GetGameInstance() const { return nullptr; }

    float    GetDeltaTime() const { return DeltaTime; }
    int64_t  GetTickCount() const { return TickCount; }

protected:
    float    DeltaTime  = 0.0f;
    int64_t  TickCount  = 0;
};

// Global engine pointer -- set by FEngineLoop::Init, cleared by Exit.
extern ENGINE_API FEngine* GEngine;

} // namespace Enigma
