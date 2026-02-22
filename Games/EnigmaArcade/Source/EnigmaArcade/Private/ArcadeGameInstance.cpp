#include "ArcadeGameInstance.h"
#include "GenericPlatform/GenericWindowDefinition.h"
#include "GenericPlatform/GenericApplication.h"
#include "GenericPlatform/GenericWindow.h"
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

	// Create a console window for ASCII rendering.
	Enigma::FWindowDefinition windowDef;
	windowDef.Title = "EnigmaArcade";
	windowDef.Width = 120;   // columns
	windowDef.Height = 40;   // rows
	windowDef.Type = Enigma::EWindowType::Console;

	Enigma::FGenericApplication* app = Enigma::FGenericApplication::GetApplication();
	if (app != nullptr)
	{
		m_consoleWindow = app->MakeWindow(windowDef);
		if (m_consoleWindow != nullptr)
		{
			m_consoleWindow->Show();
		}
	}

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

void FArcadeGameInstance::Render()
{
	if (m_consoleWindow == nullptr)
	{
		return;
	}

	// Basic ASCII frame rendering.
	std::printf("[EnigmaArcade] Frame %llu\n",
		static_cast<unsigned long long>(TickCount));
}

void FArcadeGameInstance::Shutdown()
{
	std::printf("[EnigmaArcade] Game Loop Ended (total ticks: %llu)\n",
		static_cast<unsigned long long>(TickCount));

	if (m_consoleWindow != nullptr)
	{
		Enigma::FGenericApplication* app = Enigma::FGenericApplication::GetApplication();
		if (app != nullptr)
		{
			app->DestroyWindow(m_consoleWindow);
		}
		m_consoleWindow = nullptr;
	}

	Enigma::FGameInstance::Shutdown();
}
