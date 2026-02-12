// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleInitializerEntry.h"

// ---------------------------------------------------------------
// Module registration macros (REQ-012)
//
// Self-registering pattern (mirrors Unreal Engine):
// Each module DLL defines a static FModuleInitializerEntry that
// registers itself into a global linked list when the DLL is loaded.
// The ModuleManager finds modules by name via FindModule().
//
// No extern "C" exports are used for module creation -- the
// linked list approach avoids ABI/name-mangling issues entirely.
//
// Usage (in exactly one .cpp file per module):
//   IMPLEMENT_MODULE(FCoreModule, Core)
//   IMPLEMENT_GAME_MODULE(FMyGameModule, MyGame)
//   IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, MyGame, "MyGame")
// ---------------------------------------------------------------

/// Standard engine module registration.
/// Creates a static initializer function and a self-registering
/// FModuleInitializerEntry that links into the global module list.
///
/// @param ModuleImplClass  The class implementing IModuleInterface
/// @param ModuleName       The module name (unquoted identifier)
#define IMPLEMENT_MODULE(ModuleImplClass, ModuleName)                                   \
    static ::Enigma::IModuleInterface* Initialize##ModuleName##Module()                 \
    {                                                                                   \
        return new ModuleImplClass();                                                   \
    }                                                                                   \
    static ::Enigma::FModuleInitializerEntry                                            \
        ModuleName##InitializerEntry(#ModuleName, Initialize##ModuleName##Module);

/// Game module registration.
/// Identical to IMPLEMENT_MODULE -- game vs engine distinction is
/// handled by IModuleInterface::IsGameModule() virtual method override.
/// (Matches Unreal Engine behavior where IMPLEMENT_GAME_MODULE == IMPLEMENT_MODULE)
///
/// @param ModuleImplClass  The class implementing IModuleInterface
/// @param ModuleName       The module name (unquoted identifier)
#define IMPLEMENT_GAME_MODULE(ModuleImplClass, ModuleName)                              \
    IMPLEMENT_MODULE(ModuleImplClass, ModuleName)

/// Primary game module registration (mirrors Unreal Engine).
/// Used for the main game module that typically has no custom module class.
/// The GameName parameter is kept for UE API compatibility but unused.
///
/// @param ModuleImplClass  Typically FDefaultGameModuleImpl
/// @param ModuleName       The module name (unquoted identifier)
/// @param GameName         The game name string (UE compatibility, unused)
#define IMPLEMENT_PRIMARY_GAME_MODULE(ModuleImplClass, ModuleName, GameName)            \
    IMPLEMENT_MODULE(ModuleImplClass, ModuleName)
