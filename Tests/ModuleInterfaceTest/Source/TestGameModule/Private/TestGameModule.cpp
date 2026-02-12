// Test game module implementation using IMPLEMENT_GAME_MODULE macro.

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleMacros.h"
#include <cstdio>

class FTestGameModule : public Enigma::IModuleInterface
{
public:
    void StartupModule() override
    {
        std::printf("[TestGameModule] StartupModule called\n");
    }

    void ShutdownModule() override
    {
        std::printf("[TestGameModule] ShutdownModule called\n");
    }

    bool IsGameModule() const override
    {
        return true;
    }
};

IMPLEMENT_GAME_MODULE(FTestGameModule, TestGameModule)
