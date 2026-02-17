// Copyright EnigmaEngine. All Rights Reserved.
// Core.Delegates.Tests -- TDelegate (single-cast) unit tests.

#include "Delegates/Delegate.h"

#include <gtest/gtest.h>

#include <string>

using namespace Enigma;

// -----------------------------------------------------------
// Helper types
// -----------------------------------------------------------

static int StaticAdd(int a, int b) { return a + b; }
static void StaticNoOp() {}

class FTestObject
{
public:
    int Multiply(int a, int b) { return a * b; }
    void Increment() { ++Counter; }
    int Counter = 0;
};

// -----------------------------------------------------------
// Suite: Core_Delegates_Singlecast
// -----------------------------------------------------------

TEST(Core_Delegates_Singlecast, DefaultConstructedIsUnbound)
{
    TDelegate<int(int, int)> del;
    EXPECT_FALSE(del.IsBound());
    EXPECT_FALSE(static_cast<bool>(del));
}

TEST(Core_Delegates_Singlecast, BindLambdaAndExecute)
{
    TDelegate<int(int, int)> del;
    del.Bind([](int a, int b) { return a + b; });

    EXPECT_TRUE(del.IsBound());
    EXPECT_EQ(del.Execute(3, 4), 7);
}

TEST(Core_Delegates_Singlecast, BindStaticFunctionAndExecute)
{
    TDelegate<int(int, int)> del;
    del.Bind(&StaticAdd);

    EXPECT_TRUE(del.IsBound());
    EXPECT_EQ(del.Execute(5, 6), 11);
}

TEST(Core_Delegates_Singlecast, BindMemberFunctionAndExecute)
{
    FTestObject obj;
    TDelegate<int(int, int)> del;
    del.Bind(&obj, &FTestObject::Multiply);

    EXPECT_TRUE(del.IsBound());
    EXPECT_EQ(del.Execute(3, 7), 21);
}

TEST(Core_Delegates_Singlecast, UnboundExecuteReturnsDefault)
{
    TDelegate<int()> del;
    EXPECT_EQ(del.Execute(), 0);

    TDelegate<float()> delFloat;
    EXPECT_FLOAT_EQ(delFloat.Execute(), 0.0f);

    TDelegate<std::string()> delStr;
    EXPECT_EQ(delStr.Execute(), std::string{});
}

TEST(Core_Delegates_Singlecast, UnboundVoidExecuteIsNoOp)
{
    TDelegate<void()> del;
    // Should not crash
    del.Execute();
}

TEST(Core_Delegates_Singlecast, ExecuteIfBoundReturnsTrueWhenBound)
{
    TDelegate<void()> del;
    int called = 0;
    del.Bind([&called]() { ++called; });

    EXPECT_TRUE(del.ExecuteIfBound());
    EXPECT_EQ(called, 1);
}

TEST(Core_Delegates_Singlecast, ExecuteIfBoundReturnsFalseWhenUnbound)
{
    TDelegate<void()> del;
    EXPECT_FALSE(del.ExecuteIfBound());
}

TEST(Core_Delegates_Singlecast, UnbindClearsBinding)
{
    TDelegate<int()> del;
    del.Bind([]() { return 42; });
    ASSERT_TRUE(del.IsBound());

    del.Unbind();
    EXPECT_FALSE(del.IsBound());
    EXPECT_EQ(del.Execute(), 0);
}

TEST(Core_Delegates_Singlecast, RebindOverwritesPrevious)
{
    TDelegate<int()> del;
    del.Bind([]() { return 1; });
    EXPECT_EQ(del.Execute(), 1);

    del.Bind([]() { return 2; });
    EXPECT_EQ(del.Execute(), 2);
}

TEST(Core_Delegates_Singlecast, MoveTransfersBinding)
{
    TDelegate<int()> del;
    del.Bind([]() { return 99; });

    TDelegate<int()> moved = std::move(del);
    EXPECT_TRUE(moved.IsBound());
    EXPECT_EQ(moved.Execute(), 99);
}

TEST(Core_Delegates_Singlecast, CopyPreservesBinding)
{
    TDelegate<int()> del;
    del.Bind([]() { return 55; });

    TDelegate<int()> copy = del;
    EXPECT_TRUE(copy.IsBound());
    EXPECT_EQ(copy.Execute(), 55);
    // Original still works
    EXPECT_EQ(del.Execute(), 55);
}

TEST(Core_Delegates_Singlecast, VoidMemberFunction)
{
    FTestObject obj;
    TDelegate<void()> del;
    del.Bind(&obj, &FTestObject::Increment);

    del.Execute();
    EXPECT_EQ(obj.Counter, 1);

    del.Execute();
    EXPECT_EQ(obj.Counter, 2);
}

TEST(Core_Delegates_Singlecast, OperatorBoolMatchesIsBound)
{
    TDelegate<void()> del;
    EXPECT_FALSE(static_cast<bool>(del));

    del.Bind([]() {});
    EXPECT_TRUE(static_cast<bool>(del));

    del.Unbind();
    EXPECT_FALSE(static_cast<bool>(del));
}
