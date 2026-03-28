// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "GameFramework/GameInstance.h"

#include <functional>
#include <memory>

namespace Enigma
{

class FGenericWindow;

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

    /// Get the game window created during Init. May be nullptr in headless mode.
    FGenericWindow* GetGameWindow() const { return m_gameWindow; }

    // [TEST] Recreate GameInstance after hot-reload. Destroys old instance,
    // creates new one using current factory (from reloaded DLL), calls Init.
    // Remove when Editor exists and handles object reconstruction.
    void RecreateGameInstance();

protected:
    /// Factory method -- uses registered factory if available,
    /// otherwise falls back to base FGameInstance.
    virtual std::unique_ptr<FGameInstance> CreateGameInstance();

    /// Create the game window using config values from GConfig.
    /// Reads window title, dimensions, type from INI sections.
    /// Falls back to defaults (120x40 Console) if GConfig is null.
    virtual FGenericWindow* CreateGameWindow();

private:
    std::unique_ptr<FGameInstance> GameInstance;
    FGenericWindow* m_gameWindow = nullptr;
    static GameInstanceFactory s_GameInstanceFactory;
};

} // namespace Enigma
