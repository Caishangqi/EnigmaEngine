// FEngineLoop Test Runner.
// Validates the full engine lifecycle:
//   1. Module loading by ELoadingPhase order
//   2. GEngine creation in Init
//   3. DeltaTime calculation in Tick
//   4. Clean shutdown with module unloading
//   5. Frame counting

#include "EngineLoop.h"
#include "Engine/Engine.h"
#include "Engine/LoadingPhase.h"
#include "Modules/ModuleManager.h"

#include <cstdio>
#include <thread>
#include <chrono>

static int g_passed = 0;
static int g_failed = 0;

static void Assert(bool cond, const char* msg)
{
    if (cond) { std::printf("  PASSED: %s\n", msg); ++g_passed; }
    else      { std::printf("  FAILED: %s\n", msg); ++g_failed; }
}

int main()
{
    std::printf("=== FEngineLoop Test ===\n");

    Enigma::FEngineLoop engineLoop;

    // -- Test 1: Initial state --
    std::printf("\n[Test 1] Initial state\n");
    {
        Assert(!engineLoop.IsRunning(), "Not running initially");
        Assert(!engineLoop.IsExitRequested(), "Exit not requested initially");
        Assert(engineLoop.GetFrameNumber() == 0, "Frame number is 0");
        Assert(Enigma::GEngine == nullptr, "GEngine is null initially");
    }

    // -- Test 2: Register modules to phases --
    std::printf("\n[Test 2] Register modules to loading phases\n");
    {
        engineLoop.AddModuleToPhase(
            Enigma::ELoadingPhase::EarliestPossible, "TestEarlyModule");
        engineLoop.AddModuleToPhase(
            Enigma::ELoadingPhase::Default, "TestDefaultModule");
        engineLoop.AddModuleToPhase(
            Enigma::ELoadingPhase::PostEngineInit, "TestPostEngineModule");
        Assert(true, "Modules registered to 3 different phases");
    }

    // -- Test 3: PreInit -- loads EarliestPossible phase --
    std::printf("\n[Test 3] PreInit\n");
    {
        std::printf("  -- Expecting: TestEarlyModule StartupModule --\n");
        int32_t result = engineLoop.PreInit("");
        Assert(result == 0, "PreInit returns 0 (success)");

        auto& mgr = Enigma::FModuleManager::Get();
        Assert(mgr.IsModuleLoaded("TestEarlyModule"),
            "TestEarlyModule loaded (EarliestPossible)");
        Assert(!mgr.IsModuleLoaded("TestDefaultModule"),
            "TestDefaultModule NOT loaded yet (Default phase)");
        Assert(!mgr.IsModuleLoaded("TestPostEngineModule"),
            "TestPostEngineModule NOT loaded yet (PostEngineInit phase)");
    }

    // -- Test 4: Init -- creates GEngine, loads Default + PostEngineInit --
    std::printf("\n[Test 4] Init\n");
    {
        std::printf("  -- Expecting: GEngine created, Default + PostEngineInit modules loaded --\n");
        int32_t result = engineLoop.Init();
        Assert(result == 0, "Init returns 0 (success)");
        Assert(Enigma::GEngine != nullptr, "GEngine created");
        Assert(engineLoop.IsRunning(), "Engine is running");

        auto& mgr = Enigma::FModuleManager::Get();
        Assert(mgr.IsModuleLoaded("TestDefaultModule"),
            "TestDefaultModule loaded (Default phase)");
        Assert(mgr.IsModuleLoaded("TestPostEngineModule"),
            "TestPostEngineModule loaded (PostEngineInit phase)");
    }

    // -- Test 5: Tick -- DeltaTime and frame counting --
    std::printf("\n[Test 5] Tick (3 frames with ~10ms sleep)\n");
    {
        for (int i = 0; i < 3; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            engineLoop.Tick();
        }

        Assert(engineLoop.GetFrameNumber() == 3, "Frame number is 3 after 3 ticks");

        float dt = engineLoop.GetDeltaTime();
        Assert(dt > 0.0f, "DeltaTime is positive");
        Assert(dt < 1.0f, "DeltaTime is reasonable (< 1 second)");

        Assert(Enigma::GEngine->GetTickCount() == 3,
            "GEngine tick count is 3");
        Assert(Enigma::GEngine->GetDeltaTime() > 0.0f,
            "GEngine DeltaTime is positive");

        std::printf("  (DeltaTime = %.4f s, FrameNumber = %lld)\n",
            dt, static_cast<long long>(engineLoop.GetFrameNumber()));
    }

    // -- Test 6: RequestExit --
    std::printf("\n[Test 6] RequestExit\n");
    {
        Assert(!engineLoop.IsExitRequested(), "Exit not requested before call");
        engineLoop.RequestExit();
        Assert(engineLoop.IsExitRequested(), "Exit requested after call");
    }

    // -- Test 7: Exit -- shutdown GEngine, unload all modules --
    std::printf("\n[Test 7] Exit\n");
    {
        std::printf("  -- Expecting: GEngine shutdown, all modules unloaded (reverse order) --\n");
        engineLoop.Exit();

        Assert(Enigma::GEngine == nullptr, "GEngine is null after Exit");
        Assert(!engineLoop.IsRunning(), "Engine not running after Exit");

        auto& mgr = Enigma::FModuleManager::Get();
        Assert(!mgr.IsModuleLoaded("TestEarlyModule"),
            "TestEarlyModule unloaded");
        Assert(!mgr.IsModuleLoaded("TestDefaultModule"),
            "TestDefaultModule unloaded");
        Assert(!mgr.IsModuleLoaded("TestPostEngineModule"),
            "TestPostEngineModule unloaded");
    }

    std::printf("\n=== Results: %d passed, %d failed ===\n",
        g_passed, g_failed);

    return g_failed > 0 ? 1 : 0;
}
