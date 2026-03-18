// Copyright EnigmaEngine. All Rights Reserved.

#include "TickSystem/TickFunction.h"
#include "TickSystem/TickTaskManager.h"
#include "GameFramework/Component.h"
#include "Logging/LogMacros.h"

#include <algorithm>

DEFINE_LOG_CATEGORY_STATIC(LogTickFunction, Info, All);

namespace Enigma
{

// -----------------------------------------------------------------
// FTickFunction
// -----------------------------------------------------------------

void FTickFunction::RegisterTickFunction(FTickTaskManager& manager)
{
	if (m_bRegistered)
	{
		ENIGMA_LOG(LogTickFunction, Warning,
			"FTickFunction already registered -- ignoring duplicate registration");
		return;
	}

	m_manager     = &manager;
	m_bEnabled    = bStartWithTickEnabled;
	m_bRegistered = true;

	manager.AddTickFunction(*this);
}

void FTickFunction::UnregisterTickFunction()
{
	if (!m_bRegistered || !m_manager)
	{
		return;
	}

	m_manager->RemoveTickFunction(*this);
	m_manager     = nullptr;
	m_bRegistered = false;
}

void FTickFunction::SetTickFunctionEnable(bool bEnabled)
{
	if (m_bEnabled == bEnabled)
	{
		return;
	}

	m_bEnabled = bEnabled;

	// Manager handles the move between enabled/disabled lists at frame boundary
}

void FTickFunction::AddPrerequisite(FTickFunction& other)
{
	// Avoid duplicates
	auto it = std::find(m_prerequisites.begin(), m_prerequisites.end(), &other);
	if (it != m_prerequisites.end())
	{
		return;
	}

	m_prerequisites.push_back(&other);
}

void FTickFunction::RemovePrerequisite(FTickFunction& other)
{
	auto it = std::find(m_prerequisites.begin(), m_prerequisites.end(), &other);
	if (it != m_prerequisites.end())
	{
		m_prerequisites.erase(it);
	}
}

// -----------------------------------------------------------------
// FComponentTickFunction
// -----------------------------------------------------------------

void FComponentTickFunction::ExecuteTick(float deltaTime)
{
	if (!Target)
	{
		return;
	}

	if (!Target->IsEnabled())
	{
		return;
	}

	Target->Update(deltaTime);
}

} // namespace Enigma
