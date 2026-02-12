// Copyright EnigmaEngine. All Rights Reserved.

#include "CoreMinimal.h"

namespace Enigma
{

// ---------------------------------------------------------------
// FLaunchModule -- Launch module registration
//
// The Launch module is the application entry point. It owns
// GEngineLoop and provides GuardedMain. It does not support
// dynamic reloading since it contains the main() entry point.
// ---------------------------------------------------------------
class FLaunchModule : public IModuleInterface
{
public:
    void StartupModule() override
    {
        // Launch module startup -- nothing to initialize here.
        // GEngineLoop is a global instance driven by GuardedMain.
    }

    void ShutdownModule() override
    {
        // Launch module shutdown -- cleanup handled by GuardedMain.
    }

    bool SupportsDynamicReloading() override
    {
        return false;
    }
};

IMPLEMENT_MODULE(FLaunchModule, Launch)

} // namespace Enigma
