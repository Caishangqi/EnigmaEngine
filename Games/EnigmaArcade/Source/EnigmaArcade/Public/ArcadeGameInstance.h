#pragma once

#include "EnigmaArcadeAPI.generated.h"
#include "GameFramework/GameInstance.h"
#include <cstdint>

namespace Enigma { class FGenericWindow; }

/// Game instance for EnigmaArcade.
/// Creates a console window with render-friendly mode for ASCII art output.
/// Overrides Update() for game logic and Render() for ASCII display.
class ENIGMAARCADE_API FArcadeGameInstance : public Enigma::FGameInstance
{
public:
	void Init() override;
	void Update(float deltaTime) override;
	void Render() override;
	void Shutdown() override;

	/// Current tick count (incremented each Update call).
	uint64_t GetTickCount() const { return TickCount; }

private:
	Enigma::FGenericWindow* m_consoleWindow = nullptr;
	uint64_t TickCount = 0;
};
