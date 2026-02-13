#include "ArcadeGameInstance.h"
#include <nlohmann/json.hpp>
#include <cstdio>
#include <iostream>
#include <string>

void FArcadeGameInstance::Init()
{
    Enigma::FGameInstance::Init();
    TickCount = 0;

    // REQ-014: JSON validation -- create and output config using nlohmann/json
    nlohmann::json config = {{"game", "EnigmaArcade"}, {"version", 1}};
    std::cout << "[EnigmaArcade] Config: " << config.dump() << '\n';

    std::printf("[EnigmaArcade] Engine Initialized\n");
}

void FArcadeGameInstance::Update(float deltaTime)
{
    Enigma::FGameInstance::Update(deltaTime);
    ++TickCount;
    static bool alreadyTick = false;
    if (!alreadyTick)
    {
        alreadyTick = true;
        std::printf("[EnigmaArcade] Tick: %llu\n", static_cast<unsigned long long>(TickCount));
    }
}

void FArcadeGameInstance::Shutdown()
{
    std::printf("[EnigmaArcade] Game Loop Ended (total ticks: %llu)\n", static_cast<unsigned long long>(TickCount));
    Enigma::FGameInstance::Shutdown();
}
