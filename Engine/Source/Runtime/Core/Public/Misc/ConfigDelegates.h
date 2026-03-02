// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"
#include "Delegates/MulticastDelegate.h"

#include <string>

// -------------------------------------------------------------
// ConfigDelegates.h
//
// Config system delegate events.
// Mirrors UE's FCoreDelegates config-related delegates
// (simplified subset, split into dedicated header).
// -------------------------------------------------------------

namespace Enigma
{

class FConfigFile;

/// Config system delegate events.
/// Mirrors UE's FCoreDelegates config-related delegates (simplified subset).
struct CORE_API FConfigDelegates
{
    /// Broadcast when a config domain finishes loading/merging.
    /// Params: const std::string& configName (domain name, e.g., "Engine")
    /// UE equivalent: FCoreDelegates::TSOnFConfigCreated
    using FOnConfigLoaded = TMulticastDelegate<const std::string& /*configName*/>;
    static FOnConfigLoaded OnConfigLoaded;

    /// Broadcast when config sections are modified (via SetString, etc.).
    /// Params: const std::string& configName, const std::string& sectionName
    /// UE equivalent: FCoreDelegates::TSOnConfigSectionsChanged
    using FOnConfigSectionChanged = TMulticastDelegate<const std::string& /*configName*/,
                                                        const std::string& /*sectionName*/>;
    static FOnConfigSectionChanged OnConfigSectionChanged;

    /// Broadcast when the entire config system is initialized and ready.
    /// UE equivalent: FCoreDelegates::TSConfigReadyForUse
    using FOnConfigReadyForUse = TMulticastDelegate<>;
    static FOnConfigReadyForUse OnConfigReadyForUse;
};

} // namespace Enigma
