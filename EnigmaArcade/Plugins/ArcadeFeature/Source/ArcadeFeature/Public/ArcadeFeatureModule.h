#pragma once

#include "Modules/ModuleInterface.h"

/// ArcadeFeature plugin module.
/// Loaded at PostEngineInit phase to extend EnigmaArcade with feature functionality.
class FArcadeFeatureModule : public Enigma::IModuleInterface
{
public:
    void StartupModule() override;
    void ShutdownModule() override;
};
