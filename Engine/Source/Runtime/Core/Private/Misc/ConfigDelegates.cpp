// Copyright EnigmaEngine. All Rights Reserved.

#include "Misc/ConfigDelegates.h"

namespace Enigma
{

// Static delegate instance definitions.
FConfigDelegates::FOnConfigLoaded FConfigDelegates::OnConfigLoaded;
FConfigDelegates::FOnConfigSectionChanged FConfigDelegates::OnConfigSectionChanged;
FConfigDelegates::FOnConfigReadyForUse FConfigDelegates::OnConfigReadyForUse;

} // namespace Enigma
