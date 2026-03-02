// Copyright EnigmaEngine. All Rights Reserved.

/// @file EngineGlobals.cpp
/// @brief GetRendererModule() / TryGetRendererModule() implementation.
/// Cached lazy-load pattern matching UE5.6's EngineGlobals.cpp.

#include "EngineModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/AssertionMacros.h"

namespace Enigma
{

static IRendererModule* s_cachedRendererModule = nullptr;

ENGINE_API IRendererModule& GetRendererModule()
{
	if (!s_cachedRendererModule)
	{
		FModuleManager::Get().LoadModule("Renderer");
		IModuleInterface* mod = FModuleManager::Get().GetModule("Renderer");
		checkf(mod != nullptr,
			"No renderer module loaded. Ensure AsciiRenderer DLL is in the module search path.");
		s_cachedRendererModule = static_cast<IRendererModule*>(mod);
	}
	return *s_cachedRendererModule;
}

ENGINE_API IRendererModule* TryGetRendererModule()
{
	if (!s_cachedRendererModule)
	{
		IModuleInterface* mod = FModuleManager::Get().GetModule("Renderer");
		if (mod)
		{
			s_cachedRendererModule = static_cast<IRendererModule*>(mod);
		}
	}
	return s_cachedRendererModule;
}

} // namespace Enigma
