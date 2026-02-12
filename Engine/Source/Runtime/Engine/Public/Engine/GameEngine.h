// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "GameFramework/GameInstance.h"

#include <functional>
#include <memory>

namespace Enigma
{

// ---------------------------------------------------------------
// FGameEngine -- game-mode engine (REQ-019)
//
// Owns a FGameInstance via std::unique_ptr. The instance is created
// by CreateGameInstance() during Init, initialized during Start,
// ticked each frame (BeginFrame -> Update -> Render -> EndFrame),
// and shut down during Shutdown.
//
// Game modules register a factory via RegisterGameInstanceFactory()
// in their StartupModule() to provide a custom FGameInstance subclass.
// ---------------------------------------------------------------
class ENGINE_API FGameEngine : public FEngine
{
public:
    /// Factory function type for creating custom FGameInstance subclasses.
    using GameInstanceFactory = std::function<std::unique_ptr<FGameInstance>()>;

    /// Register a factory that CreateGameInstance() will use.
    /// Call this from your game module's StartupModule().
    static void RegisterGameInstanceFactory(GameInstanceFactory factory);

    void Init(FEngineLoop* engineLoop) override;
    void Start() override;
    void Tick(float deltaTime) override;
    void Shutdown() override;

    FGameInstance* GetGameInstance() const override;

protected:
    /// Factory method -- uses registered factory if available,
    /// otherwise falls back to base FGameInstance.
    virtual std::unique_ptr<FGameInstance> CreateGameInstance();

private:
    std::unique_ptr<FGameInstance> GameInstance;
    static GameInstanceFactory s_GameInstanceFactory;
};

} // namespace Enigma
