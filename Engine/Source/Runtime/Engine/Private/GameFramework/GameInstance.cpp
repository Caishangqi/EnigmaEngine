// Copyright EnigmaEngine. All Rights Reserved.

#include "GameFramework/GameInstance.h"

#include <cstdio>

namespace Enigma
{

void FGameInstance::Init()
{
    std::printf("[FGameInstance] Init\n");
}

void FGameInstance::Shutdown()
{
    std::printf("[FGameInstance] Shutdown\n");
}

void FGameInstance::BeginFrame()
{
    ++FrameCount;
}

void FGameInstance::Update(float deltaTime)
{
    DeltaTime = deltaTime;
    // Default: no-op. Users override for game logic.
}

void FGameInstance::Render()
{
    // Default: no-op. Users override for rendering.
}

void FGameInstance::EndFrame()
{
    // Default: no-op. Users override for end-of-frame cleanup.
}

} // namespace Enigma
