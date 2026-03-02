// Copyright EnigmaEngine. All Rights Reserved.

#include "Engine/EngineDelegates.h"

namespace Enigma
{

// Static delegate instance definitions.
FEngineDelegates::FOnGameWindowCreated FEngineDelegates::OnGameWindowCreated;
FEngineDelegates::FOnGameWindowDestroyed FEngineDelegates::OnGameWindowDestroyed;
FEngineDelegates::FOnPreRender FEngineDelegates::OnPreRender;
FEngineDelegates::FOnPostRender FEngineDelegates::OnPostRender;

} // namespace Enigma
