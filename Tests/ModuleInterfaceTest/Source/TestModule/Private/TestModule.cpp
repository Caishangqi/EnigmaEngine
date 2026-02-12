// Test module implementation using IMPLEMENT_MODULE macro.

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleMacros.h"
#include <cstdio>

class FTestModule : public Enigma::IModuleInterface
{
public:
    void StartupModule() override
    {
        std::printf("[TestModule] StartupModule called\n");
    }

    void ShutdownModule() override
    {
        std::printf("[TestModule] ShutdownModule called\n");
    }

    bool SupportsDynamicReloading() override
    {
        return false;
    }
};

IMPLEMENT_MODULE(FTestModule, TestModule)
