// Copyright EnigmaEngine. All Rights Reserved.
// Core.Delegates.Tests -- TMulticastDelegate unit tests.

#include "Delegates/MulticastDelegate.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace Enigma;

// -----------------------------------------------------------
// Helper types
// -----------------------------------------------------------

class FListener
{
public:
    void OnEvent(int value) { Values.push_back(value); }
    void OnSimple() { ++CallCount; }

    std::vector<int> Values;
    int CallCount = 0;
};

// -----------------------------------------------------------
// Suite: Core_Delegates_Multicast
// -----------------------------------------------------------

TEST(Core_Delegates_Multicast, DefaultConstructedIsEmpty)
{
    TMulticastDelegate<int> del;
    EXPECT_FALSE(del.IsBound());
    EXPECT_EQ(del.GetCount(), 0);
}

TEST(Core_Delegates_Multicast, AddReturnsValidHandle)
{
    TMulticastDelegate<> del;
    FDelegateHandle handle = del.Add([]() {});

    EXPECT_TRUE(handle.IsValid());
    EXPECT_EQ(del.GetCount(), 1);
}

TEST(Core_Delegates_Multicast, BroadcastCallsAllListeners)
{
    int count = 0;
    TMulticastDelegate<> del;

    del.Add([&count]() { ++count; });
    del.Add([&count]() { ++count; });
    del.Add([&count]() { ++count; });

    del.Broadcast();
    EXPECT_EQ(count, 3);
}

TEST(Core_Delegates_Multicast, BroadcastCallsInRegistrationOrder)
{
    std::vector<int> order;
    TMulticastDelegate<> del;

    del.Add([&order]() { order.push_back(1); });
    del.Add([&order]() { order.push_back(2); });
    del.Add([&order]() { order.push_back(3); });

    del.Broadcast();

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

TEST(Core_Delegates_Multicast, BroadcastPassesArguments)
{
    int received = 0;
    TMulticastDelegate<int> del;

    del.Add([&received](int val) { received = val; });
    del.Broadcast(42);

    EXPECT_EQ(received, 42);
}

TEST(Core_Delegates_Multicast, RemoveByHandleStopsBroadcast)
{
    int count = 0;
    TMulticastDelegate<> del;

    FDelegateHandle h1 = del.Add([&count]() { ++count; });
    FDelegateHandle h2 = del.Add([&count]() { ++count; });

    EXPECT_TRUE(del.Remove(h1));
    EXPECT_EQ(del.GetCount(), 1);

    del.Broadcast();
    EXPECT_EQ(count, 1);
}

TEST(Core_Delegates_Multicast, RemoveInvalidHandleReturnsFalse)
{
    TMulticastDelegate<> del;
    del.Add([]() {});

    FDelegateHandle invalid;
    EXPECT_FALSE(del.Remove(invalid));

    FDelegateHandle unrelated = FDelegateHandle::Generate();
    EXPECT_FALSE(del.Remove(unrelated));
}

TEST(Core_Delegates_Multicast, RemoveAllByObject)
{
    FListener listenerA;
    FListener listenerB;
    TMulticastDelegate<> del;

    del.Add(&listenerA, &FListener::OnSimple);
    del.Add(&listenerA, &FListener::OnSimple);
    del.Add(&listenerB, &FListener::OnSimple);

    EXPECT_EQ(del.GetCount(), 3);

    del.RemoveAll(&listenerA);
    EXPECT_EQ(del.GetCount(), 1);

    del.Broadcast();
    EXPECT_EQ(listenerA.CallCount, 0);
    EXPECT_EQ(listenerB.CallCount, 1);
}

TEST(Core_Delegates_Multicast, ClearRemovesAll)
{
    TMulticastDelegate<> del;
    del.Add([]() {});
    del.Add([]() {});
    del.Add([]() {});

    ASSERT_EQ(del.GetCount(), 3);

    del.Clear();
    EXPECT_EQ(del.GetCount(), 0);
    EXPECT_FALSE(del.IsBound());
}

TEST(Core_Delegates_Multicast, BroadcastDuringModifySafe)
{
    // Listener that removes itself during broadcast.
    // Copy-on-iterate ensures no UB.
    TMulticastDelegate<> del;
    FDelegateHandle selfHandle;
    int callCount = 0;

    selfHandle = del.Add([&]()
    {
        ++callCount;
        del.Remove(selfHandle);
    });
    del.Add([&callCount]() { ++callCount; });

    // Should not crash; both listeners called from the copy
    del.Broadcast();
    EXPECT_EQ(callCount, 2);
    EXPECT_EQ(del.GetCount(), 1);  // self-removed
}

TEST(Core_Delegates_Multicast, BroadcastDuringAddSafe)
{
    // Listener that adds a new listener during broadcast.
    TMulticastDelegate<> del;
    int callCount = 0;

    del.Add([&]()
    {
        ++callCount;
        del.Add([&callCount]() { ++callCount; });
    });

    del.Broadcast();
    // Only the original listener should have been called (copy-on-iterate)
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(del.GetCount(), 2);  // new one added

    // Second broadcast calls both
    callCount = 0;
    del.Broadcast();
    EXPECT_EQ(callCount, 2);
}

TEST(Core_Delegates_Multicast, MemberFunctionBinding)
{
    FListener listener;
    TMulticastDelegate<int> del;

    del.Add(&listener, &FListener::OnEvent);
    del.Broadcast(10);
    del.Broadcast(20);

    ASSERT_EQ(listener.Values.size(), 2u);
    EXPECT_EQ(listener.Values[0], 10);
    EXPECT_EQ(listener.Values[1], 20);
}

TEST(Core_Delegates_Multicast, MoveSemantics)
{
    int count = 0;
    TMulticastDelegate<> del;
    del.Add([&count]() { ++count; });

    TMulticastDelegate<> moved = std::move(del);
    EXPECT_TRUE(moved.IsBound());

    moved.Broadcast();
    EXPECT_EQ(count, 1);
}

TEST(Core_Delegates_Multicast, MultipleArguments)
{
    float resultA = 0.0f;
    float resultB = 0.0f;
    TMulticastDelegate<float, float> del;

    del.Add([&resultA](float a, float b) { resultA = a + b; });
    del.Add([&resultB](float a, float b) { resultB = a * b; });

    del.Broadcast(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(resultA, 7.0f);
    EXPECT_FLOAT_EQ(resultB, 12.0f);
}
