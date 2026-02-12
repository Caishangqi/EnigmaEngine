// Copyright EnigmaEngine. All Rights Reserved.
//
// Task 4.2 Test: FGameEngine <-> FGameInstance Frame Loop Connection
//
// Validates:
//   [1]  Default FGameEngine creates FGameInstance via CreateGameInstance
//   [2]  GetGameInstance() returns non-null after Init
//   [3]  Tick drives BeginFrame->Update->Render->EndFrame order
//   [4]  DeltaTime passed to Update matches Tick argument
//   [5]  FrameCount increments in BeginFrame (per 4.1)
//   [6]  Multiple Ticks accumulate FrameCount correctly
//   [7]  Shutdown destroys GameInstance (GetGameInstance returns null)
//   [8]  Custom FGameEngine overrides CreateGameInstance
//   [9]  Custom GameInstance receives all frame calls
//   [10] Custom GameInstance Update receives correct DeltaTime
//   [11] Custom GameInstance FrameCount tracks correctly
//   [12] Custom GameInstance Init called during Start
//   [13] Custom GameInstance Shutdown called during Shutdown
//   [14] FEngine::Tick base class TickCount incremented
//   [15] FEngine::GetDeltaTime matches last Tick dt

#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "GameFramework/GameInstance.h"
#include "EngineLoop.h"

#include <cstdio>
#include <cmath>
#include <memory>

static int g_passed = 0;
static int g_failed = 0;

static void Check(bool cond, const char* name)
{
    if (cond) { std::printf("  [PASS] %s\n", name); ++g_passed; }
    else      { std::printf("  [FAIL] %s\n", name); ++g_failed; }
}

// ---- Custom GameInstance to track calls ----
class FCustomGameInstance : public Enigma::FGameInstance
{
public:
    bool bInitCalled     = false;
    bool bShutdownCalled = false;
    int  beginCount      = 0;
    int  updateCount     = 0;
    int  renderCount     = 0;
    int  endCount        = 0;
    float lastDt         = -1.0f;

    // Track call order within a single frame
    int callOrder[4]     = {0, 0, 0, 0}; // begin, update, render, end
    int orderCounter     = 0;

    void Init() override
    {
        FGameInstance::Init();
        bInitCalled = true;
    }

    void Shutdown() override
    {
        FGameInstance::Shutdown();
        bShutdownCalled = true;
    }

    void BeginFrame() override
    {
        FGameInstance::BeginFrame();
        ++beginCount;
        orderCounter = 0;
        callOrder[0] = ++orderCounter;
    }

    void Update(float deltaTime) override
    {
        FGameInstance::Update(deltaTime);
        ++updateCount;
        lastDt = deltaTime;
        callOrder[1] = ++orderCounter;
    }

    void Render() override
    {
        FGameInstance::Render();
        ++renderCount;
        callOrder[2] = ++orderCounter;
    }

    void EndFrame() override
    {
        FGameInstance::EndFrame();
        ++endCount;
        callOrder[3] = ++orderCounter;
    }
};

// ---- Custom GameEngine that overrides CreateGameInstance ----
class FCustomGameEngine : public Enigma::FGameEngine
{
public:
    // Expose the custom instance for test inspection
    FCustomGameInstance* CustomInstance = nullptr;

protected:
    std::unique_ptr<Enigma::FGameInstance> CreateGameInstance() override
    {
        auto inst = std::make_unique<FCustomGameInstance>();
        CustomInstance = inst.get();
        return inst;
    }
};

int main()
{
    std::printf("=== Task 4.2: FGameEngine <-> FGameInstance Connection Test ===\n\n");

    // ---- Part A: Default FGameEngine ----
    std::printf("[Part A] Default FGameEngine with base FGameInstance\n");
    {
        Enigma::FGameEngine engine;
        Enigma::FEngineLoop loop;

        engine.Init(&loop);
        auto* gi = engine.GetGameInstance();
        Check(gi != nullptr,
            "[1]  Default FGameEngine creates FGameInstance");
        Check(gi != nullptr,
            "[2]  GetGameInstance() returns non-null after Init");

        engine.Start();

        // Tick once
        engine.Tick(0.016f);

        if (gi)
        {
            Check(gi->GetFrameCount() == 1,
                "[5]  FrameCount increments in BeginFrame");
            Check(std::fabs(gi->GetDeltaTime() - 0.016f) < 1e-6f,
                "[4]  DeltaTime passed to Update matches Tick arg");
        }

        // Tick 4 more times
        engine.Tick(0.017f);
        engine.Tick(0.015f);
        engine.Tick(0.016f);
        engine.Tick(0.018f);

        if (gi)
        {
            Check(gi->GetFrameCount() == 5,
                "[6]  Multiple Ticks: FrameCount = 5 after 5 ticks");
        }

        // Base class tracking
        Check(engine.GetTickCount() == 5,
            "[14] FEngine::Tick base TickCount = 5");
        Check(std::fabs(engine.GetDeltaTime() - 0.018f) < 1e-6f,
            "[15] FEngine::GetDeltaTime matches last Tick dt");

        engine.Shutdown();
        Check(engine.GetGameInstance() == nullptr,
            "[7]  Shutdown destroys GameInstance (null)");
    }

    // ---- Part B: Custom FGameEngine with custom FGameInstance ----
    std::printf("\n[Part B] Custom FGameEngine with FCustomGameInstance\n");
    {
        FCustomGameEngine engine;
        Enigma::FEngineLoop loop;

        engine.Init(&loop);
        Check(engine.CustomInstance != nullptr,
            "[8]  Custom FGameEngine overrides CreateGameInstance");

        auto* ci = engine.CustomInstance;

        engine.Start();
        Check(ci->bInitCalled,
            "[12] Custom GameInstance Init called during Start");

        // Tick once and verify call order
        engine.Tick(0.033f);

        Check(ci->beginCount == 1 && ci->updateCount == 1
           && ci->renderCount == 1 && ci->endCount == 1,
            "[9]  Custom GameInstance receives all frame calls");

        // Verify order: Begin(1) -> Update(2) -> Render(3) -> End(4)
        Check(ci->callOrder[0] == 1 && ci->callOrder[1] == 2
           && ci->callOrder[2] == 3 && ci->callOrder[3] == 4,
            "[3]  Tick drives BeginFrame->Update->Render->EndFrame order");

        Check(std::fabs(ci->lastDt - 0.033f) < 1e-6f,
            "[10] Custom GameInstance Update receives correct DeltaTime");

        // Tick 9 more times
        for (int i = 0; i < 9; ++i)
            engine.Tick(0.016f);

        Check(ci->GetFrameCount() == 10,
            "[11] Custom GameInstance FrameCount = 10 after 10 ticks");

        engine.Shutdown();
        Check(ci->bShutdownCalled,
            "[13] Custom GameInstance Shutdown called during Shutdown");
    }

    // ---- Summary ----
    std::printf("\n=== %d/%d tests passed ===\n",
        g_passed, g_passed + g_failed);

    return g_failed > 0 ? 1 : 0;
}
