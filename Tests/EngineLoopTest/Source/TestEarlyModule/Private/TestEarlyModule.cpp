// TestEarlyModule -- loaded in EarliestPossible phase.

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleMacros.h"
#include <cstdio>

class FTestEarlyModule : public Enigma::IModuleInterface
{
public:
    void StartupModule() override
    {
        std::printf("[TestEarlyModule] StartupModule (EarliestPossible)\n");
    }
    void ShutdownModule() override
    {
        std::printf("[TestEarlyModule] ShutdownModule\n");
    }
};

IMPLEMENT_MODULE(FTestEarlyModule, TestEarlyModule)
