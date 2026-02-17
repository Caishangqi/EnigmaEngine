// Copyright EnigmaEngine. All Rights Reserved.
// Core.Delegates.Tests -- FDelegateHandle unit tests.

#include "Delegates/DelegateHandle.h"

#include <gtest/gtest.h>

#include <set>

using namespace Enigma;

// -----------------------------------------------------------
// Suite: Core_Delegates_Handle
// -----------------------------------------------------------

TEST(Core_Delegates_Handle, DefaultConstructedIsInvalid)
{
    FDelegateHandle handle;
    EXPECT_FALSE(handle.IsValid());
}

TEST(Core_Delegates_Handle, GenerateReturnsValidHandle)
{
    FDelegateHandle handle = FDelegateHandle::Generate();
    EXPECT_TRUE(handle.IsValid());
}

TEST(Core_Delegates_Handle, GenerateReturnsUniqueIds)
{
    constexpr int kCount = 100;
    std::set<FDelegateHandle> handles;

    // Use a set with a custom comparator is not possible since
    // FDelegateHandle only has == / !=. Use a vector + uniqueness check.
    std::vector<FDelegateHandle> vec;
    vec.reserve(kCount);

    for (int i = 0; i < kCount; ++i)
    {
        vec.push_back(FDelegateHandle::Generate());
    }

    // Verify all handles are unique (O(n^2) but small n)
    for (int i = 0; i < kCount; ++i)
    {
        for (int j = i + 1; j < kCount; ++j)
        {
            EXPECT_NE(vec[i], vec[j]) << "Handles at index " << i << " and " << j << " are equal";
        }
    }
}

TEST(Core_Delegates_Handle, ResetMakesInvalid)
{
    FDelegateHandle handle = FDelegateHandle::Generate();
    ASSERT_TRUE(handle.IsValid());

    handle.Reset();
    EXPECT_FALSE(handle.IsValid());
}

TEST(Core_Delegates_Handle, EqualityOperator)
{
    FDelegateHandle a = FDelegateHandle::Generate();
    FDelegateHandle b = a;  // copy

    EXPECT_EQ(a, b);
}

TEST(Core_Delegates_Handle, InequalityOperator)
{
    FDelegateHandle a = FDelegateHandle::Generate();
    FDelegateHandle b = FDelegateHandle::Generate();

    EXPECT_NE(a, b);
}

TEST(Core_Delegates_Handle, DefaultConstructedHandlesAreEqual)
{
    FDelegateHandle a;
    FDelegateHandle b;

    EXPECT_EQ(a, b);
}

TEST(Core_Delegates_Handle, CopyPreservesValidity)
{
    FDelegateHandle original = FDelegateHandle::Generate();
    FDelegateHandle copy = original;

    EXPECT_TRUE(copy.IsValid());
    EXPECT_EQ(original, copy);
}

TEST(Core_Delegates_Handle, MoveTransfersState)
{
    FDelegateHandle original = FDelegateHandle::Generate();
    FDelegateHandle moved = std::move(original);

    EXPECT_TRUE(moved.IsValid());
}
