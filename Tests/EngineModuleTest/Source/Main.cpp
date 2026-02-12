// Engine Module Test Runner.
// Validates:
//   1. Engine.dll compiles with all ENGINE_API exports
//   2. FModuleManager loads Core before Engine (dependency order)
//   3. FEngineModule StartupModule/ShutdownModule lifecycle
//   4. ENGINE_API symbols accessible (FEngineLoop, FEngine, FGameEngine, FGameInstance)
//   5. Clean unload

#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "EngineLoop.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "Engine/GameInstance.h"

#include <cstdio>

static int g_passed = 0;
static int g_failed = 0;

static void Assert(bool cond, const char* msg)
{
    if (cond) { std::printf("  PASSED: %s\n", msg); ++g_passed; }
    else      { std::printf("  FAILED: %s\n", msg); ++g_failed; }
}

int main()
{
    std::printf("=== Engine Module Test ===\n");

    auto& mgr = Enigma::FModuleManager::Get();

    // -- Test 1: Load Core first (dependency) --
    std::printf("\n[Test 1] Load Core module\n");
    {
        auto* core = mgr.LoadModule("Core");
        Assert(core != nullptr, "Core loaded");
    }

    // -- Test 2: Load Engine module --
    std::printf("\n[Test 2] Load Engine module\n");
    {
        auto* engine = mgr.LoadModule("Engine");
        Assert(engine != nullptr, "Engine loaded");
        Assert(mgr.IsModuleLoaded("Engine"), "IsModuleLoaded('Engine') is true");
    }

    // -- Test 3: Engine module properties --
    std::printf("\n[Test 3] Engine module properties\n");
    {
        auto* mod = mgr.GetModule("Engine");
        Assert(mod != nullptr, "GetModule('Engine') non-null");
        Assert(mod->SupportsDynamicReloading() == false,
            "Engine does not support dynamic reloading");
        Assert(mod->IsGameModule() == false,
            "Engine is not a game module");
    }

    // -- Test 4: ENGINE_API symbols accessible --
    std::printf("\n[Test 4] ENGINE_API exported symbols\n");
    {
        // FEngineLoop (stack-allocated, ENGINE_API class)
        Enigma::FEngineLoop loop;
        Assert(loop.GetFrameNumber() == 0, "FEngineLoop accessible");

        // FGameEngine (heap-allocated, ENGINE_API class)
        auto* ge = new Enigma::FGameEngine();
        Assert(ge != nullptr, "FGameEngine constructible");
        Assert(ge->GetGameInstance() == nullptr,
            "FGameEngine::GetGameInstance null before Init");
        delete ge;

        // FGameInstance (heap-allocated, ENGINE_API class)
        auto* gi = new Enigma::FGameInstance();
        Assert(gi != nullptr, "FGameInstance constructible");
        Assert(gi->GetFrameCount() == 0, "FGameInstance accessible");
        delete gi;

        // GEngine global pointer
        Assert(Enigma::GEngine == nullptr,
            "GEngine accessible (null before Init)");
    }

    // -- Test 5: Unload Engine then Core --
    std::printf("\n[Test 5] Unload in reverse order\n");
    {
        bool okE = mgr.UnloadModule("Engine");
        Assert(okE, "Engine unloaded");
        Assert(!mgr.IsModuleLoaded("Engine"), "Engine no longer loaded");

        bool okC = mgr.UnloadModule("Core");
        Assert(okC, "Core unloaded");
    }

    std::printf("\n=== Results: %d passed, %d failed ===\n",
        g_passed, g_failed);

    return g_failed > 0 ? 1 : 0;
}
