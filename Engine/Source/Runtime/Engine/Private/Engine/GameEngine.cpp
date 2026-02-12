// Copyright EnigmaEngine. All Rights Reserved.

#include "Engine/GameEngine.h"
#include "EngineLoop.h"

#include <cstdio>

namespace Enigma
{

// Static factory -- defaults to null (base FGameInstance fallback)
FGameEngine::GameInstanceFactory FGameEngine::s_GameInstanceFactory = nullptr;

void FGameEngine::RegisterGameInstanceFactory(GameInstanceFactory factory)
{
    s_GameInstanceFactory = std::move(factory);
}

void FGameEngine::Init(FEngineLoop* engineLoop)
{
    FEngine::Init(engineLoop);

    // Create the game instance via factory method
    GameInstance = CreateGameInstance();
    if (GameInstance)
    {
        std::printf("[FGameEngine] GameInstance created\n");
    }
    else
    {
        std::fprintf(stderr,
            "[FGameEngine] WARNING: CreateGameInstance returned null\n");
    }

    std::printf("[FGameEngine] Init\n");
}

void FGameEngine::Start()
{
    FEngine::Start();

    // Initialize the game instance
    if (GameInstance)
    {
        GameInstance->Init();
    }

    std::printf("[FGameEngine] Start\n");
}

void FGameEngine::Tick(float deltaTime)
{
    FEngine::Tick(deltaTime);

    // Drive the game instance frame loop
    if (GameInstance)
    {
        GameInstance->BeginFrame();
        GameInstance->Update(deltaTime);
        GameInstance->Render();
        GameInstance->EndFrame();
    }
}

void FGameEngine::Shutdown()
{
    std::printf("[FGameEngine] Shutdown\n");

    // Shutdown and release the game instance
    if (GameInstance)
    {
        GameInstance->Shutdown();
        GameInstance.reset();
        std::printf("[FGameEngine] GameInstance destroyed\n");
    }

    FEngine::Shutdown();
}

FGameInstance* FGameEngine::GetGameInstance() const
{
    return GameInstance.get();
}

std::unique_ptr<FGameInstance> FGameEngine::CreateGameInstance()
{
    if (s_GameInstanceFactory)
    {
        return s_GameInstanceFactory();
    }
    return std::make_unique<FGameInstance>();
}

} // namespace Enigma
