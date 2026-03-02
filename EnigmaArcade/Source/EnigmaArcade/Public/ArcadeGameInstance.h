#pragma once

#include "EnigmaArcadeAPI.generated.h"
#include "GameFramework/GameInstance.h"
#include <cstdint>

/// Game instance for EnigmaArcade.
/// Uses the renderer API for ASCII art output.
/// Demonstrates moving lit-cell pattern with camera following.
/// Window creation is handled by FGameEngine (config-driven).
class ENIGMAARCADE_API FArcadeGameInstance : public Enigma::FGameInstance
{
public:
	void Init() override;
	void Update(float deltaTime) override;
	void Render() override;
	void Shutdown() override;

private:
	uint64_t TickCount = 0;

	/// Current lit column for the moving lit-cell demo pattern.
	int32_t m_litColumn = 0;
};
