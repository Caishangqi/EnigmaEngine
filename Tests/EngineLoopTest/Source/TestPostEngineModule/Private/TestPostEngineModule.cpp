// TestPostEngineModule -- loaded in PostEngineInit phase.

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleMacros.h"
#include <cstdio>

class FTestPostEngineModule : public Enigma::IModuleInterface
{
public:
    void StartupModule() override
    {
        std::printf("[TestPostEngineModule] StartupModule (PostEngineInit)\n");
    }
    void ShutdownModule() override
    {
        std::printf("[TestPostEngineModule] ShutdownModule\n");
    }
};

IMPLEMENT_MODULE(FTestPostEngineModule, TestPostEngineModule)
