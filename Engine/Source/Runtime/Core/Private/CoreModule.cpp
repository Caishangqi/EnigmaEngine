// Copyright EnigmaEngine. All Rights Reserved.

// Core module implementation.
// Core is the lowest-level engine module -- it has zero dependencies
// and provides the module system, platform abstraction, and base types.

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleMacros.h"

#include <cstdio>

class FCoreModule : public Enigma::IModuleInterface
{
public:
    void StartupModule() override
    {
        std::printf("[Core] StartupModule\n");
    }

    void ShutdownModule() override
    {
        std::printf("[Core] ShutdownModule\n");
    }

    bool SupportsDynamicReloading() override
    {
        // Core is always loaded -- never hot-reloaded.
        return false;
    }
};

IMPLEMENT_MODULE(FCoreModule, Core)
