// Copyright EnigmaEngine. All Rights Reserved.
//
// FGameInstance Base Class Test (Task 4.1)
//
// Validates:
//   [1]  FGameInstance default constructs with FrameCount=0, DeltaTime=0
//   [2]  Init() callable on base class
//   [3]  Shutdown() callable on base class
//   [4]  BeginFrame() increments FrameCount
//   [5]  Update(dt) stores DeltaTime
//   [6]  Render() callable (no-op)
//   [7]  EndFrame() callable (no-op)
//   [8]  Full frame loop: BeginFrame->Update->Render->EndFrame
//   [9]  FrameCount increments each BeginFrame call
//   [10] DeltaTime updated each Update call
//   [11] Virtual override: custom subclass BeginFrame called
//   [12] Virtual override: custom subclass Update called
//   [13] Virtual override: custom subclass Render called
//   [14] Virtual override: custom subclass EndFrame called
//   [15] Virtual override: custom subclass Init called
//   [16] Virtual override: custom subclass Shutdown called
//   [17] GetFrameCount() accessor correct
//   [18] GetDeltaTime() accessor correct
//   [19] Multiple frames: FrameCount matches frame count
//   [20] DeltaTime reflects last Update value

#include "GameFramework/GameInstance.h"

#include <cstdio>
#include <cmath>

static int g_passed = 0;
static int g_failed = 0;

static void Check(bool cond, const char* name)
{
    if (cond) { std::printf("  [PASS] %s\n", name); ++g_passed; }
    else      { std::printf("  [FAIL] %s\n", name); ++g_failed; }
}

// Custom subclass to verify virtual dispatch
class FTestGameInstance : public Enigma::FGameInstance
{
public:
    bool bInitCalled     = false;
    bool bShutdownCalled = false;
    bool bBeginCalled    = false;
    bool bUpdateCalled   = false;
    bool bRenderCalled   = false;
    bool bEndCalled      = false;
    float lastDt         = -1.0f;
    int   customCounter  = 0;

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
        FGameInstance::BeginFrame();  // increments FrameCount
        bBeginCalled = true;
    }

    void Update(float deltaTime) override
    {
        FGameInstance::Update(deltaTime);  // stores DeltaTime
        bUpdateCalled = true;
        lastDt = deltaTime;
        ++customCounter;
    }

    void Render() override
    {
        FGameInstance::Render();
        bRenderCalled = true;
    }

    void EndFrame() override
    {
        FGameInstance::EndFrame();
        bEndCalled = true;
    }
};

int main()
{
    std::printf("=== FGameInstance Base Class Test (Task 4.1) ===\n\n");

    // ---- Test 1: Default construction ----
    std::printf("[Test 1-2-3] Base class lifecycle\n");
    {
        Enigma::FGameInstance gi;
        Check(gi.GetFrameCount() == 0 && gi.GetDeltaTime() == 0.0f,
            "[1]  Default: FrameCount=0, DeltaTime=0");

        gi.Init();
        Check(true, "[2]  Init() callable on base class");

        gi.Shutdown();
        Check(true, "[3]  Shutdown() callable on base class");
    }

    // ---- Test 4-7: Individual frame methods ----
    std::printf("\n[Test 4-7] Individual frame methods\n");
    {
        Enigma::FGameInstance gi;

        gi.BeginFrame();
        Check(gi.GetFrameCount() == 1,
            "[4]  BeginFrame() increments FrameCount (0->1)");

        gi.Update(0.016f);
        Check(std::fabs(gi.GetDeltaTime() - 0.016f) < 1e-6f,
            "[5]  Update(0.016) stores DeltaTime");

        gi.Render();
        Check(true, "[6]  Render() callable (no-op)");

        gi.EndFrame();
        Check(true, "[7]  EndFrame() callable (no-op)");
    }

    // ---- Test 8-10: Full frame loop ----
    std::printf("\n[Test 8-10] Full frame loop\n");
    {
        Enigma::FGameInstance gi;

        // Simulate 3 frames
        float dts[] = {0.016f, 0.033f, 0.008f};
        for (int i = 0; i < 3; ++i)
        {
            gi.BeginFrame();
            gi.Update(dts[i]);
            gi.Render();
            gi.EndFrame();
        }

        Check(true, "[8]  Full frame loop: BeginFrame->Update->Render->EndFrame");

        Check(gi.GetFrameCount() == 3,
            "[9]  FrameCount increments each BeginFrame (3 frames -> 3)");

        Check(std::fabs(gi.GetDeltaTime() - 0.008f) < 1e-6f,
            "[10] DeltaTime updated each Update (last = 0.008)");
    }

    // ---- Test 11-16: Virtual override ----
    std::printf("\n[Test 11-16] Virtual override (FTestGameInstance)\n");
    {
        FTestGameInstance tgi;

        // Call through base pointer to verify virtual dispatch
        Enigma::FGameInstance* base = &tgi;

        base->Init();
        Check(tgi.bInitCalled,
            "[15] Virtual override: custom Init called");

        base->BeginFrame();
        Check(tgi.bBeginCalled && tgi.GetFrameCount() == 1,
            "[11] Virtual override: custom BeginFrame called");

        base->Update(0.025f);
        Check(tgi.bUpdateCalled && std::fabs(tgi.lastDt - 0.025f) < 1e-6f,
            "[12] Virtual override: custom Update called");

        base->Render();
        Check(tgi.bRenderCalled,
            "[13] Virtual override: custom Render called");

        base->EndFrame();
        Check(tgi.bEndCalled,
            "[14] Virtual override: custom EndFrame called");

        base->Shutdown();
        Check(tgi.bShutdownCalled,
            "[16] Virtual override: custom Shutdown called");
    }

    // ---- Test 17-20: Accessors and multi-frame ----
    std::printf("\n[Test 17-20] Accessors and multi-frame\n");
    {
        FTestGameInstance tgi;
        Enigma::FGameInstance* base = &tgi;

        // Run 10 frames
        for (int i = 0; i < 10; ++i)
        {
            float dt = 0.001f * (i + 1);
            base->BeginFrame();
            base->Update(dt);
            base->Render();
            base->EndFrame();
        }

        Check(base->GetFrameCount() == 10,
            "[17] GetFrameCount() accessor = 10 after 10 frames");

        Check(std::fabs(base->GetDeltaTime() - 0.010f) < 1e-6f,
            "[18] GetDeltaTime() accessor = 0.010 (last frame dt)");

        Check(tgi.GetFrameCount() == 10,
            "[19] Multiple frames: FrameCount matches frame count");

        Check(tgi.customCounter == 10,
            "[20] Custom counter incremented 10 times via virtual Update");
    }

    // ---- Summary ----
    std::printf("\n=== %d/%d tests passed ===\n",
        g_passed, g_passed + g_failed);

    return g_failed > 0 ? 1 : 0;
}
