// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "Engine/GameEngine.h"
#include <memory>

// ---------------------------------------------------------------
// REGISTER_GAME_INSTANCE macro
//
// Registers a custom FGameInstance subclass via a static initializer.
// The engine will create an instance of this class instead of the
// default FGameInstance.
//
// Usage (in the primary game module .cpp, after IMPLEMENT_PRIMARY_GAME_MODULE):
//   REGISTER_GAME_INSTANCE(FMyGameInstance)
// ---------------------------------------------------------------

namespace Enigma::Detail
{

/// Helper that registers a GameInstance factory during static initialization.
template <typename T>
struct FGameInstanceRegistrar
{
    FGameInstanceRegistrar()
    {
        FGameEngine::RegisterGameInstanceFactory([]() {
            return std::make_unique<T>();
        });
    }
};

} // namespace Enigma::Detail

/// Register a custom FGameInstance subclass.
/// Place this in the primary game module .cpp alongside IMPLEMENT_PRIMARY_GAME_MODULE.
///
/// @param GameInstanceClass  The class deriving from Enigma::FGameInstance
#define REGISTER_GAME_INSTANCE(GameInstanceClass)                                       \
    static ::Enigma::Detail::FGameInstanceRegistrar<GameInstanceClass>                  \
        s_GameInstanceRegistrar_##GameInstanceClass;
