// Copyright EnigmaEngine. All Rights Reserved.

#include "Launch.h"
#include "LaunchEngineLoop.h"
#include "CoreGlobals.h"

#include <cstdio>

namespace Enigma
{

// ---------------------------------------------------------------
// Global FEngineLoop instance
// ---------------------------------------------------------------
LAUNCH_API FEngineLoop GEngineLoop;

// ---------------------------------------------------------------
// GuardedMain
// ---------------------------------------------------------------
int32_t GuardedMain(const char* cmdLine)
{
    std::printf("[GuardedMain] Engine startup\n");

    // ---- Phase 1: PreInit ----
    int32_t result = GEngineLoop.PreInit(cmdLine);
    if (result != 0)
    {
        std::fprintf(stderr, "[GuardedMain] PreInit failed (code %d)\n", result);
        return result;
    }

    // ---- Phase 2: Init ----
    result = GEngineLoop.Init();
    if (result != 0)
    {
        std::fprintf(stderr, "[GuardedMain] Init failed (code %d)\n", result);
        GEngineLoop.Exit();
        return result;
    }

    // ---- Phase 3: Main loop ----
    std::printf("[GuardedMain] Entering main loop\n");
    while (!IsEngineExitRequested())
    {
        GEngineLoop.Tick();
    }
    std::printf("[GuardedMain] Main loop ended (frame %lld)\n",
        static_cast<long long>(GEngineLoop.GetFrameNumber()));

    // ---- Phase 4: Exit ----
    GEngineLoop.Exit();

    std::printf("[GuardedMain] Engine shutdown complete\n");
    return 0;
}

} // namespace Enigma
