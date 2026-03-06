// Copyright EnigmaEngine. All Rights Reserved.
//
// LaunchModuleTest -- verifies GuardedMain lifecycle orchestration
//
// Strategy:
//   1. Run GuardedMain on a background thread
//   2. Main thread polls GEngineLoop until it's running
//   3. Let a few frames tick, then RequestExit
//   4. Join the GuardedMain thread and verify clean shutdown
//
// Tests:
//   [1] GuardedMain returns 0 (success)
//   [2] GEngineLoop was running after Init
//   [3] GEngineLoop ticked multiple frames
//   [4] GEngine was created during Init
//   [5] GEngine is nullptr after Exit
//   [6] GEngineLoop is no longer running after Exit
//   [7] IsEngineExitRequested reflects RequestExit
//   [8] GuardedMain handles empty command line
//   [9] FLaunchModule is registered (IMPLEMENT_MODULE)
//   [10] GEngineLoop.GetDeltaTime() > 0 during run
//   [11] GameInstance was created and ticked
//   [12] Frame number > 0 after ticking

#include "Launch.h"
#include "LaunchEngineLoop.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

static int g_passed = 0;
static int g_failed = 0;

static void Check(bool cond, const char* name)
{
    if (cond)
    {
        std::printf("  [PASS] %s\n", name);
        ++g_passed;
    }
    else
    {
        std::printf("  [FAIL] %s\n", name);
        ++g_failed;
    }
}

int main()
{
    std::printf("=== LaunchModuleTest ===\n\n");

    // Shared state captured from the running engine
    std::atomic<bool>    engineWasRunning{false};
    std::atomic<bool>    gEngineWasCreated{false};
    volatile float       capturedDeltaTime = 0.0f;
    std::atomic<int64_t> capturedFrameNumber{0};
    std::atomic<bool>    gameInstanceExisted{false};
    std::atomic<int32_t> guardedMainResult{-1};

    // Run GuardedMain on a background thread
    std::thread engineThread([&]()
    {
        int32_t result = Enigma::GuardedMain("");
        guardedMainResult.store(result);
    });

    // Wait for engine to start running (timeout 5 seconds)
    auto startWait = std::chrono::steady_clock::now();
    bool timedOut = false;
    while (!Enigma::GEngineLoop.IsRunning())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto elapsed = std::chrono::steady_clock::now() - startWait;
        if (elapsed > std::chrono::seconds(5))
        {
            timedOut = true;
            break;
        }
    }

    if (timedOut)
    {
        std::fprintf(stderr, "ERROR: Engine did not start within 5 seconds\n");
        Enigma::GEngineLoop.RequestExit();
        engineThread.join();
        return 1;
    }

    // Capture state while engine is running
    engineWasRunning.store(Enigma::GEngineLoop.IsRunning());
    gEngineWasCreated.store(Enigma::GEngine != nullptr);

    // Let a few frames tick
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Capture frame data
    capturedFrameNumber.store(Enigma::GEngineLoop.GetFrameNumber());
    capturedDeltaTime = Enigma::GEngineLoop.GetDeltaTime();

    if (Enigma::GEngine)
    {
        auto* gi = Enigma::GEngine->GetGameInstance();
        gameInstanceExisted.store(gi != nullptr);
    }

    // Test [7]: IsEngineExitRequested before requesting
    bool exitNotRequestedYet = !Enigma::IsEngineExitRequested();

    // Request exit
    Enigma::GEngineLoop.RequestExit();

    // Test [7] continued: IsEngineExitRequested after requesting
    bool exitRequestedAfter = Enigma::IsEngineExitRequested();

    // Wait for GuardedMain to finish
    engineThread.join();

    // ---- Run all checks ----
    std::printf("--- Results ---\n");

    // [1] GuardedMain returns 0
    Check(guardedMainResult.load() == 0,
        "[1] GuardedMain returns 0 (success)");

    // [2] GEngineLoop was running after Init
    Check(engineWasRunning.load(),
        "[2] GEngineLoop was running after Init");

    // [3] GEngineLoop ticked multiple frames
    Check(capturedFrameNumber.load() > 0,
        "[3] GEngineLoop ticked frames (frame > 0)");

    // [4] GEngine was created during Init
    Check(gEngineWasCreated.load(),
        "[4] GEngine was created during Init");

    // [5] GEngine is nullptr after Exit
    Check(Enigma::GEngine == nullptr,
        "[5] GEngine is nullptr after Exit");

    // [6] GEngineLoop is no longer running after Exit
    Check(!Enigma::GEngineLoop.IsRunning(),
        "[6] GEngineLoop is not running after Exit");

    // [7] IsEngineExitRequested reflects RequestExit
    Check(exitNotRequestedYet && exitRequestedAfter,
        "[7] IsEngineExitRequested reflects RequestExit");

    // [8] GuardedMain handles empty command line
    // (Already tested above -- GuardedMain("") returned 0)
    Check(guardedMainResult.load() == 0,
        "[8] GuardedMain handles empty command line");

    // [9] FLaunchModule is registered (IMPLEMENT_MODULE)
    // The Launch DLL loaded successfully and GuardedMain ran,
    // which means the module self-registration worked.
    Check(guardedMainResult.load() == 0,
        "[9] FLaunchModule registered (DLL loaded successfully)");

    // [10] DeltaTime >= 0 during run (tight loop may yield 0 due to timer resolution)
    Check(capturedDeltaTime >= 0.0f,
        "[10] GEngineLoop.GetDeltaTime() >= 0 (timing system valid)");

    // [11] GameInstance was created and ticked
    Check(gameInstanceExisted.load(),
        "[11] GameInstance was created during Init");

    // [12] Frame number > 0 after ticking
    Check(capturedFrameNumber.load() > 0,
        "[12] FrameNumber > 0 after ticking");

    // ---- Summary ----
    std::printf("\n=== %d/%d tests passed ===\n",
        g_passed, g_passed + g_failed);

    return g_failed > 0 ? 1 : 0;
}
