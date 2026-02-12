// Copyright EnigmaEngine. All Rights Reserved.

// Engine module implementation.
// Depends on Core. Provides FEngineLoop, FEngine, FGameEngine, FGameInstance.

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleMacros.h"

#include <cstdio>

class FEngineModule : public Enigma::IModuleInterface
{
public:
    void StartupModule() override
    {
        std::printf("[Engine] StartupModule\n");
    }

    void ShutdownModule() override
    {
        std::printf("[Engine] ShutdownModule\n");
    }

    bool SupportsDynamicReloading() override
    {
        // Engine is always loaded -- never hot-reloaded.
        return false;
    }
};

IMPLEMENT_MODULE(FEngineModule, Engine)
