#pragma once

#include "EnigmaArcadeAPI.generated.h"
#include "GameFramework/GameInstance.h"
#include <cstdint>

/// Game instance for EnigmaArcade.
/// Overrides Update() to output an incrementing tick counter,
/// verifying the game loop is running correctly (REQ-013).
class ENIGMAARCADE_API FArcadeGameInstance : public Enigma::FGameInstance
{
public:
    void Init() override;
    void Update(float deltaTime) override;
    void Shutdown() override;

    /// Current tick count (incremented each Update call).
    uint64_t GetTickCount() const { return TickCount; }

private:
    uint64_t TickCount = 0;
};
