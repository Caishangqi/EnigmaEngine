// Copyright EnigmaEngine. All Rights Reserved.

// ApplicationCore module implementation.
// Provides platform-agnostic application lifecycle, window management,
// and OS message pump abstractions.

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleMacros.h"

#include <cstdio>

class FApplicationCoreModule : public Enigma::IModuleInterface
{
public:
    void StartupModule() override
    {
        std::printf("[ApplicationCore] StartupModule\n");
    }

    void ShutdownModule() override
    {
        std::printf("[ApplicationCore] ShutdownModule\n");
    }

    bool SupportsDynamicReloading() override
    {
        return false;
    }
};

IMPLEMENT_MODULE(FApplicationCoreModule, ApplicationCore)
