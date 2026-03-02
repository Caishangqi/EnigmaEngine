// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file EngineModule.h
/// @brief Cached lazy-loading accessors for the active renderer module.
/// Matches UE5.6's EngineModule.h pattern (Engine/Public/EngineModule.h).

#include "EngineAPI.generated.h"
#include "RenderCore/RendererInterface.h"

namespace Enigma
{

/// Retrieve the active renderer module via cached lazy-load.
/// Uses FModuleManager::LoadModule() + GetModule() + checkf + static_cast.
/// Asserts (via checkf) if no renderer module is loaded.
ENGINE_API IRendererModule& GetRendererModule();

/// Non-asserting variant -- returns nullptr if no renderer is loaded.
/// Used by FGameEngine::Tick() for backward-compatible fallback.
ENGINE_API IRendererModule* TryGetRendererModule();

} // namespace Enigma
