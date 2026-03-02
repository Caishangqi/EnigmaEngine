#include "ArcadeFeatureModule.h"
#include "Modules/ModuleMacros.h"
#include <cstdio>

void FArcadeFeatureModule::StartupModule()
{
    std::printf("[ArcadeFeature] Plugin loaded at PostEngineInit\n");
}

void FArcadeFeatureModule::ShutdownModule()
{
    std::printf("[ArcadeFeature] Plugin unloaded\n");
}

IMPLEMENT_MODULE(FArcadeFeatureModule, ArcadeFeature)
