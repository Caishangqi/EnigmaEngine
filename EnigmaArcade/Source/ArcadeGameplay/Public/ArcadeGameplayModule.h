#pragma once

#include "ArcadeGameplayAPI.generated.h"
#include "Modules/ModuleInterface.h"


/// ArcadeGameplay module: demonstrates module lifecycle with owned resources.
class ARCADEGAMEPLAY_API FArcadeGameplayModule : public Enigma::IModuleInterface
{
public:
    void StartupModule() override;
    void ShutdownModule() override;

private:
};
