// FEngine / FGameEngine / FGameInstance Test Runner.
// Validates:
//   1. GEngine creation and global accessibility
//   2. FGameEngine owns FGameInstance via unique_ptr
//   3. GetGameInstance() returns correct pointer
//   4. Tick propagates to GameInstance (BeginFrame/Update/Render/EndFrame)
//   5. GameInstance frame count and DeltaTime tracking
//   6. Clean shutdown destroys GameInstance
//   7. FEngine base returns nullptr for GetGameInstance

#include "EngineLoop.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "Engine/GameInstance.h"

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
    std::printf("=== FEngine / FGameEngine / FGameInstance Test ===\n");

    Enigma::FEngineLoop engineLoop;

    // -- Test 1: GEngine null before Init --
    std::printf("\n[Test 1] Pre-Init state\n");
    {
        Assert(Enigma::GEngine == nullptr, "GEngine is null before Init");
    }

    // -- Test 2: FEngine base class GetGameInstance --
    std::printf("\n[Test 2] FEngine base GetGameInstance\n");
    {
        Enigma::FEngine baseEngine;
        Assert(baseEngine.GetGameInstance() == nullptr,
            "FEngine::GetGameInstance returns nullptr");
        Assert(baseEngine.GetDeltaTime() == 0.0f,
            "FEngine DeltaTime initially 0");
        Assert(baseEngine.GetTickCount() == 0,
            "FEngine TickCount initially 0");
    }

    // -- Test 3: PreInit + Init creates GEngine with GameInstance --
    std::printf("\n[Test 3] Init creates GEngine with GameInstance\n");
    {
        engineLoop.PreInit("");
        engineLoop.Init();

        Assert(Enigma::GEngine != nullptr, "GEngine created");

        auto* gi = Enigma::GEngine->GetGameInstance();
        Assert(gi != nullptr, "GetGameInstance returns non-null");
        Assert(gi->GetFrameCount() == 0, "GameInstance frame count is 0");
        Assert(gi->GetDeltaTime() == 0.0f, "GameInstance DeltaTime is 0");
    }

    // -- Test 4: Tick propagates to GameInstance --
    std::printf("\n[Test 4] Tick propagates to GameInstance\n");
    {
        auto* gi = Enigma::GEngine->GetGameInstance();

        // Tick 5 frames with ~5ms sleep
        for (int i = 0; i < 5; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            engineLoop.Tick();
        }

        Assert(gi->GetFrameCount() == 5,
            "GameInstance frame count is 5 after 5 ticks");
        Assert(gi->GetDeltaTime() > 0.0f,
            "GameInstance DeltaTime is positive");

        Assert(Enigma::GEngine->GetTickCount() == 5,
            "GEngine tick count is 5");
        Assert(Enigma::GEngine->GetDeltaTime() > 0.0f,
            "GEngine DeltaTime is positive");

        std::printf("  (GameInstance: frames=%llu, dt=%.4f)\n",
            static_cast<unsigned long long>(gi->GetFrameCount()),
            gi->GetDeltaTime());
    }

    // -- Test 5: GameInstance identity through GEngine --
    std::printf("\n[Test 5] GameInstance identity\n");
    {
        auto* gi1 = Enigma::GEngine->GetGameInstance();
        auto* gi2 = Enigma::GEngine->GetGameInstance();
        Assert(gi1 == gi2, "GetGameInstance returns same pointer");
        Assert(gi1 != nullptr, "GameInstance is not null");
    }

    // -- Test 6: GEngine is FGameEngine --
    std::printf("\n[Test 6] GEngine is FGameEngine\n");
    {
        auto* gameEngine = dynamic_cast<Enigma::FGameEngine*>(Enigma::GEngine);
        Assert(gameEngine != nullptr, "GEngine is a FGameEngine");
    }

    // -- Test 7: Exit destroys GameInstance and GEngine --
    std::printf("\n[Test 7] Exit cleanup\n");
    {
        engineLoop.Exit();

        Assert(Enigma::GEngine == nullptr, "GEngine is null after Exit");
    }

    std::printf("\n=== Results: %d passed, %d failed ===\n",
        g_passed, g_failed);

    return g_failed > 0 ? 1 : 0;
}
