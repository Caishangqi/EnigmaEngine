#include "ArcadeGameplayModule.h"
#include "Modules/ModuleMacros.h"
#include <cstdio>

void FArcadeGameplayModule::StartupModule()
{
    std::printf("[ArcadeGameplay] StartupModule\n");
}

void FArcadeGameplayModule::ShutdownModule()
{
    std::printf("[ArcadeGameplay] ShutdownModule\n");
}

IMPLEMENT_MODULE(FArcadeGameplayModule, ArcadeGameplay)
