// Copyright EnigmaEngine. All Rights Reserved.

#include "CoreGlobals.h"

#include <cstdio>

namespace Enigma
{

CORE_API FConfigCacheIni* GConfig = nullptr;

// -----------------------------------------------------------------
// Engine exit request
// -----------------------------------------------------------------

static bool GIsRequestingExit = false;

void RequestEngineExit(const char* reasonString)
{
	if (!GIsRequestingExit)
	{
		GIsRequestingExit = true;
		std::printf("[Core] Engine exit requested: %s\n", reasonString ? reasonString : "unknown");
	}
}

bool IsEngineExitRequested()
{
	return GIsRequestingExit;
}

} // namespace Enigma
