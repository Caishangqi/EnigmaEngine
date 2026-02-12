// Copyright EnigmaEngine. All Rights Reserved.

#include "Engine/Engine.h"

#include <cstdio>

namespace Enigma
{

// Global engine pointer
FEngine* GEngine = nullptr;

void FEngine::Init(FEngineLoop* /*engineLoop*/)
{
    std::printf("[FEngine] Init\n");
}

void FEngine::Start()
{
    std::printf("[FEngine] Start\n");
}

void FEngine::Tick(float deltaTime)
{
    DeltaTime = deltaTime;
    ++TickCount;
}

void FEngine::Shutdown()
{
    std::printf("[FEngine] Shutdown\n");
}

} // namespace Enigma
