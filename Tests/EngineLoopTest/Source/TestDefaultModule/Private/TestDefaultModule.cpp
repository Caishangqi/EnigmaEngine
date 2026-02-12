// TestDefaultModule -- loaded in Default phase.

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleMacros.h"
#include <cstdio>

class FTestDefaultModule : public Enigma::IModuleInterface
{
public:
    void StartupModule() override
    {
        std::printf("[TestDefaultModule] StartupModule (Default)\n");
    }
    void ShutdownModule() override
    {
        std::printf("[TestDefaultModule] ShutdownModule\n");
    }
};

IMPLEMENT_MODULE(FTestDefaultModule, TestDefaultModule)
