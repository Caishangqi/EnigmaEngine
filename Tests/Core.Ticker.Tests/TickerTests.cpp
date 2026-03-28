// Copyright EnigmaEngine. All Rights Reserved.
// Core.Ticker.Tests -- Unit tests for FTSTicker and FTSTickerObjectBase.

#include "Containers/Ticker.h"

#include <gtest/gtest.h>

using namespace Enigma;

// ---------------------------------------------------------------
// Test fixture: resets the core ticker before each test.
// ---------------------------------------------------------------
class TickerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        FTSTicker::GetCoreTicker().Reset();
    }

    void TearDown() override
    {
        FTSTicker::GetCoreTicker().Reset();
    }

    FTSTicker& Ticker()
    {
        return FTSTicker::GetCoreTicker();
    }
};

// ---------------------------------------------------------------
// AddTicker_OneShot: delegate returns false, fires only once.
// ---------------------------------------------------------------
TEST_F(TickerTest, AddTicker_OneShot)
{
    int CallCount = 0;
    FTickerDelegate Del;
    Del.Bind([&CallCount](float) -> bool
    {
        ++CallCount;
        return false;
    });

    Ticker().AddTicker(Del);

    Ticker().Tick(0.016f);
    EXPECT_EQ(CallCount, 1);

    Ticker().Tick(0.016f);
    EXPECT_EQ(CallCount, 1);  // Should not fire again
}

// ---------------------------------------------------------------
// AddTicker_Recurring: delegate returns true, fires every tick.
// ---------------------------------------------------------------
TEST_F(TickerTest, AddTicker_Recurring)
{
    int CallCount = 0;
    FTickerDelegate Del;
    Del.Bind([&CallCount](float) -> bool
    {
        ++CallCount;
        return true;
    });

    Ticker().AddTicker(Del);

    Ticker().Tick(0.016f);
    Ticker().Tick(0.016f);
    Ticker().Tick(0.016f);
    EXPECT_EQ(CallCount, 3);
}

// ---------------------------------------------------------------
// AddTicker_WithDelay: delegate does not fire until delay elapsed.
// ---------------------------------------------------------------
TEST_F(TickerTest, AddTicker_WithDelay)
{
    int CallCount = 0;
    FTickerDelegate Del;
    Del.Bind([&CallCount](float) -> bool
    {
        ++CallCount;
        return false;
    });

    Ticker().AddTicker(Del, 0.5f);

    // Tick with small dt -- should not fire yet.
    Ticker().Tick(0.1f);
    EXPECT_EQ(CallCount, 0);

    Ticker().Tick(0.1f);
    EXPECT_EQ(CallCount, 0);

    // Tick past the delay threshold.
    Ticker().Tick(0.4f);
    EXPECT_EQ(CallCount, 1);

    // One-shot: should not fire again.
    Ticker().Tick(1.0f);
    EXPECT_EQ(CallCount, 1);
}

// ---------------------------------------------------------------
// AddTicker_RecurringWithDelay: reschedules at same delay.
// ---------------------------------------------------------------
TEST_F(TickerTest, AddTicker_RecurringWithDelay)
{
    int CallCount = 0;
    FTickerDelegate Del;
    Del.Bind([&CallCount](float) -> bool
    {
        ++CallCount;
        return true;
    });

    Ticker().AddTicker(Del, 1.0f);

    Ticker().Tick(0.5f);
    EXPECT_EQ(CallCount, 0);

    Ticker().Tick(0.6f);  // CurrentTime = 1.1, fires
    EXPECT_EQ(CallCount, 1);

    Ticker().Tick(0.5f);  // CurrentTime = 1.6, next fire at 2.1
    EXPECT_EQ(CallCount, 1);

    Ticker().Tick(0.6f);  // CurrentTime = 2.2, fires
    EXPECT_EQ(CallCount, 2);
}

// ---------------------------------------------------------------
// RemoveTicker: unregistered delegate does not fire.
// ---------------------------------------------------------------
TEST_F(TickerTest, RemoveTicker)
{
    int CallCount = 0;
    FTickerDelegate Del;
    Del.Bind([&CallCount](float) -> bool
    {
        ++CallCount;
        return true;
    });

    auto Handle = Ticker().AddTicker(Del);

    Ticker().Tick(0.016f);
    EXPECT_EQ(CallCount, 1);

    FTSTicker::RemoveTicker(Handle);

    Ticker().Tick(0.016f);
    EXPECT_EQ(CallCount, 1);  // Should not fire after removal
}

// ---------------------------------------------------------------
// RemoveTicker_BeforeFirstTick: remove from pending queue.
// ---------------------------------------------------------------
TEST_F(TickerTest, RemoveTicker_BeforeFirstTick)
{
    int CallCount = 0;
    FTickerDelegate Del;
    Del.Bind([&CallCount](float) -> bool
    {
        ++CallCount;
        return true;
    });

    auto Handle = Ticker().AddTicker(Del);
    FTSTicker::RemoveTicker(Handle);

    Ticker().Tick(0.016f);
    EXPECT_EQ(CallCount, 0);
}

// ---------------------------------------------------------------
// Tick_MultipleElements: multiple callbacks all fire.
// ---------------------------------------------------------------
TEST_F(TickerTest, Tick_MultipleElements)
{
    int CountA = 0;
    int CountB = 0;
    int CountC = 0;

    FTickerDelegate DelA;
    DelA.Bind([&CountA](float) -> bool { ++CountA; return true; });

    FTickerDelegate DelB;
    DelB.Bind([&CountB](float) -> bool { ++CountB; return false; });

    FTickerDelegate DelC;
    DelC.Bind([&CountC](float) -> bool { ++CountC; return true; });

    Ticker().AddTicker(DelA);
    Ticker().AddTicker(DelB);
    Ticker().AddTicker(DelC);

    Ticker().Tick(0.016f);
    EXPECT_EQ(CountA, 1);
    EXPECT_EQ(CountB, 1);
    EXPECT_EQ(CountC, 1);

    Ticker().Tick(0.016f);
    EXPECT_EQ(CountA, 2);
    EXPECT_EQ(CountB, 1);  // One-shot, should not fire again
    EXPECT_EQ(CountC, 2);
}

// ---------------------------------------------------------------
// TickerObjectBase: auto-register on construction, auto-unregister
// on destruction.
// ---------------------------------------------------------------
class TestTickerObject : public FTSTickerObjectBase
{
public:
    int TickCount = 0;

    TestTickerObject(float Delay = 0.0f)
        : FTSTickerObjectBase(Delay)
    {
    }

    bool Tick(float DeltaTime) override
    {
        ++TickCount;
        return true;
    }
};

TEST_F(TickerTest, TickerObjectBase_AutoRegister)
{
    {
        TestTickerObject Obj;

        Ticker().Tick(0.016f);
        EXPECT_EQ(Obj.TickCount, 1);

        Ticker().Tick(0.016f);
        EXPECT_EQ(Obj.TickCount, 2);
    }
    // Obj destroyed -- should auto-unregister.

    // Tick should not crash or fire the destroyed object.
    Ticker().Tick(0.016f);
}

// ---------------------------------------------------------------
// TickerObjectBase_WithDelay: respects delay parameter.
// ---------------------------------------------------------------
TEST_F(TickerTest, TickerObjectBase_WithDelay)
{
    TestTickerObject Obj(0.5f);

    Ticker().Tick(0.1f);
    EXPECT_EQ(Obj.TickCount, 0);

    Ticker().Tick(0.5f);
    EXPECT_EQ(Obj.TickCount, 1);
}

// ---------------------------------------------------------------
// DeltaTime_PassedToDelegate: verify correct dt is forwarded.
// ---------------------------------------------------------------
TEST_F(TickerTest, DeltaTime_PassedToDelegate)
{
    float ReceivedDt = 0.0f;
    FTickerDelegate Del;
    Del.Bind([&ReceivedDt](float dt) -> bool
    {
        ReceivedDt = dt;
        return false;
    });

    Ticker().AddTicker(Del);
    Ticker().Tick(0.033f);
    EXPECT_FLOAT_EQ(ReceivedDt, 0.033f);
}

// ---------------------------------------------------------------
// Reset_ClearsAll: after Reset(), no delegates fire.
// ---------------------------------------------------------------
TEST_F(TickerTest, Reset_ClearsAll)
{
    int CallCount = 0;
    FTickerDelegate Del;
    Del.Bind([&CallCount](float) -> bool
    {
        ++CallCount;
        return true;
    });

    Ticker().AddTicker(Del);
    Ticker().Tick(0.016f);
    EXPECT_EQ(CallCount, 1);

    Ticker().Reset();

    Ticker().Tick(0.016f);
    EXPECT_EQ(CallCount, 1);  // Should not fire after reset
}
