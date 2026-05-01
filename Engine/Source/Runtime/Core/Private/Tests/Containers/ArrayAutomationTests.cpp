// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTest.h"
#include "Containers/Array.h"

#include <cstdint>
#include <utility>

namespace Enigma
{

namespace
{

struct FTrackedLifetime
{
	explicit FTrackedLifetime(int32_t InValue = 0)
		: Value(InValue)
	{
		++LiveCount;
		++ConstructedCount;
	}

	FTrackedLifetime(const FTrackedLifetime& Other)
		: Value(Other.Value)
	{
		++LiveCount;
		++ConstructedCount;
		++CopyConstructedCount;
	}

	FTrackedLifetime(FTrackedLifetime&& Other) noexcept
		: Value(Other.Value)
	{
		Other.Value = -1;
		++LiveCount;
		++ConstructedCount;
		++MoveConstructedCount;
	}

	~FTrackedLifetime()
	{
		--LiveCount;
		++DestructedCount;
	}

	FTrackedLifetime& operator=(const FTrackedLifetime&) = delete;
	FTrackedLifetime& operator=(FTrackedLifetime&&) = delete;

	static void ResetStats()
	{
		LiveCount = 0;
		ConstructedCount = 0;
		CopyConstructedCount = 0;
		MoveConstructedCount = 0;
		DestructedCount = 0;
	}

	int32_t Value = 0;

	inline static int32_t LiveCount = 0;
	inline static int32_t ConstructedCount = 0;
	inline static int32_t CopyConstructedCount = 0;
	inline static int32_t MoveConstructedCount = 0;
	inline static int32_t DestructedCount = 0;
};

struct FMoveOnlyValue
{
	explicit FMoveOnlyValue(int32_t InValue)
		: Value(InValue)
	{
	}

	FMoveOnlyValue(const FMoveOnlyValue&) = delete;
	FMoveOnlyValue& operator=(const FMoveOnlyValue&) = delete;

	FMoveOnlyValue(FMoveOnlyValue&& Other) noexcept
		: Value(Other.Value)
	{
		Other.Value = -1;
	}

	FMoveOnlyValue& operator=(FMoveOnlyValue&&) = delete;

	int32_t Value = 0;
};

} // namespace

#define ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(TestClass, PrettyName) \
	ENIGMA_IMPLEMENT_AUTOMATION_TEST(                                       \
		TestClass,                                                          \
		PrettyName,                                                         \
		Core,                                                               \
		EAutomationTestType::Unit,                                          \
		EAutomationTestFlags::None)

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayDefaultConstructionTest,
	"System.Core.Containers.Array.DefaultConstruction")
{
	TArray<int32_t> Array;

	TestEqual("Default array should have zero elements", Array.Num(), 0);
	TestEqual("Default array should have zero capacity", Array.Max(), 0);
	TestTrue("Default array should be empty", Array.IsEmpty());
	TestTrue("Default array should not allocate storage", Array.GetData() == nullptr);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayInitializerListConstructionTest,
	"System.Core.Containers.Array.InitializerListConstruction")
{
	TArray<int32_t> Array{1, 2, 3};

	TestEqual("Initializer list should set element count", Array.Num(), 3);
	TestEqual("Initializer list should allocate exact phase 1 capacity", Array.Max(), 3);
	TestEqual("First value should be copied", Array.GetData()[0], 1);
	TestEqual("Second value should be copied", Array.GetData()[1], 2);
	TestEqual("Third value should be copied", Array.GetData()[2], 3);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayCopyConstructionTest,
	"System.Core.Containers.Array.CopyConstruction")
{
	TArray<int32_t> Source{4, 5, 6};
	TArray<int32_t> Copy(Source);

	TestEqual("Copy should preserve count", Copy.Num(), Source.Num());
	TestEqual("Copy should preserve capacity", Copy.Max(), Source.Num());
	TestTrue("Copy should own separate storage", Copy.GetData() != Source.GetData());
	TestEqual("Copy should preserve first value", Copy.GetData()[0], 4);
	TestEqual("Copy should preserve last value", Copy.GetData()[2], 6);

	Copy.GetData()[0] = 40;
	TestEqual("Mutating copy should not affect source", Source.GetData()[0], 4);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayMoveConstructionTest,
	"System.Core.Containers.Array.MoveConstruction")
{
	TArray<int32_t> Source{7, 8};
	int32_t* SourceData = Source.GetData();

	TArray<int32_t> Moved(std::move(Source));

	TestEqual("Move should preserve count", Moved.Num(), 2);
	TestEqual("Move should preserve capacity", Moved.Max(), 2);
	TestTrue("Move should steal storage", Moved.GetData() == SourceData);
	TestEqual("Moved first value should be preserved", Moved.GetData()[0], 7);
	TestEqual("Moved second value should be preserved", Moved.GetData()[1], 8);
	TestEqual("Moved-from source should be empty", Source.Num(), 0);
	TestEqual("Moved-from source should have zero capacity", Source.Max(), 0);
	TestTrue("Moved-from source should release ownership", Source.GetData() == nullptr);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayCopyAssignmentTest,
	"System.Core.Containers.Array.CopyAssignment")
{
	TArray<int32_t> Source{10, 11, 12};
	TArray<int32_t> Target{1};

	Target = Source;

	TestEqual("Copy assignment should replace count", Target.Num(), 3);
	TestEqual("Copy assignment should preserve first value", Target.GetData()[0], 10);
	TestEqual("Copy assignment should preserve last value", Target.GetData()[2], 12);

	Target.GetData()[1] = 110;
	TestEqual("Mutating target should not affect source", Source.GetData()[1], 11);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayMoveAssignmentTest,
	"System.Core.Containers.Array.MoveAssignment")
{
	TArray<int32_t> Source{20, 21};
	TArray<int32_t> Target{1, 2, 3};
	int32_t* SourceData = Source.GetData();

	Target = std::move(Source);

	TestEqual("Move assignment should replace count", Target.Num(), 2);
	TestEqual("Move assignment should preserve first value", Target.GetData()[0], 20);
	TestEqual("Move assignment should preserve second value", Target.GetData()[1], 21);
	TestTrue("Move assignment should steal source storage", Target.GetData() == SourceData);
	TestEqual("Moved-from source should be empty", Source.Num(), 0);
	TestEqual("Moved-from source should have zero capacity", Source.Max(), 0);
	TestTrue("Moved-from source should release ownership", Source.GetData() == nullptr);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayReserveGrowsAndPreservesElementsTest,
	"System.Core.Containers.Array.ReserveGrowsAndPreservesElements")
{
	TArray<int32_t> Array{30, 31};

	Array.Reserve(8);

	TestEqual("Reserve should preserve element count", Array.Num(), 2);
	TestEqual("Reserve should grow capacity", Array.Max(), 8);
	TestEqual("Reserve should preserve first value", Array.GetData()[0], 30);
	TestEqual("Reserve should preserve second value", Array.GetData()[1], 31);

	Array.Reserve(4);

	TestEqual("Reserve below capacity should not shrink", Array.Max(), 8);
	TestEqual("Reserve below capacity should preserve count", Array.Num(), 2);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayResetDestroysElementsAndKeepsStorageTest,
	"System.Core.Containers.Array.ResetDestroysElementsAndKeepsStorage")
{
	FTrackedLifetime::ResetStats();

	TArray<FTrackedLifetime> Array{FTrackedLifetime(1), FTrackedLifetime(2)};
	FTrackedLifetime* Storage = Array.GetData();
	const int32_t Capacity = Array.Max();
	const int32_t DestructedBeforeReset = FTrackedLifetime::DestructedCount;

	Array.Reset();

	TestEqual("Reset should clear count", Array.Num(), 0);
	TestEqual("Reset should preserve capacity", Array.Max(), Capacity);
	TestTrue("Reset should preserve storage", Array.GetData() == Storage);
	TestEqual("Reset should destroy each live element", FTrackedLifetime::DestructedCount, DestructedBeforeReset + 2);
	TestEqual("Reset should leave no live tracked elements", FTrackedLifetime::LiveCount, 0);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayEmptyDestroysElementsAndReleasesStorageTest,
	"System.Core.Containers.Array.EmptyDestroysElementsAndReleasesStorage")
{
	FTrackedLifetime::ResetStats();

	TArray<FTrackedLifetime> Array{FTrackedLifetime(3), FTrackedLifetime(4)};
	const int32_t DestructedBeforeEmpty = FTrackedLifetime::DestructedCount;

	Array.Empty();

	TestEqual("Empty should clear count", Array.Num(), 0);
	TestEqual("Empty should clear capacity", Array.Max(), 0);
	TestTrue("Empty should release storage", Array.GetData() == nullptr);
	TestEqual("Empty should destroy each live element", FTrackedLifetime::DestructedCount, DestructedBeforeEmpty + 2);
	TestEqual("Empty should leave no live tracked elements", FTrackedLifetime::LiveCount, 0);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayDestructorDestroysElementsTest,
	"System.Core.Containers.Array.DestructorDestroysElements")
{
	FTrackedLifetime::ResetStats();

	{
		TArray<FTrackedLifetime> Array{FTrackedLifetime(5), FTrackedLifetime(6), FTrackedLifetime(7)};
		TestEqual("Array should own three tracked elements", Array.Num(), 3);
		TestEqual("Tracked elements should be live inside array scope", FTrackedLifetime::LiveCount, 3);
	}

	TestEqual("Destructor should release all tracked elements", FTrackedLifetime::LiveCount, 0);
	TestEqual(
		"Every tracked construction should have matching destruction",
		FTrackedLifetime::DestructedCount,
		FTrackedLifetime::ConstructedCount);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayReserveMovesNonTrivialElementsTest,
	"System.Core.Containers.Array.ReserveMovesNonTrivialElements")
{
	FTrackedLifetime::ResetStats();

	TArray<FTrackedLifetime> Array{FTrackedLifetime(8), FTrackedLifetime(9)};
	const int32_t MovesBeforeReserve = FTrackedLifetime::MoveConstructedCount;
	const int32_t DestructedBeforeReserve = FTrackedLifetime::DestructedCount;

	Array.Reserve(6);

	TestEqual("Reserve should move each live element once", FTrackedLifetime::MoveConstructedCount, MovesBeforeReserve + 2);
	TestEqual("Reserve should destroy old moved-from elements", FTrackedLifetime::DestructedCount, DestructedBeforeReserve + 2);
	TestEqual("Reserve should preserve live count", FTrackedLifetime::LiveCount, 2);
	TestEqual("Reserve should preserve first tracked value", Array.GetData()[0].Value, 8);
	TestEqual("Reserve should preserve second tracked value", Array.GetData()[1].Value, 9);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayCheckedAccessTest,
	"System.Core.Containers.Array.CheckedAccess")
{
	TArray<int32_t> Array{10, 20, 30};
	const TArray<int32_t>& ConstArray = Array;

	TestTrue("First index should be valid", Array.IsValidIndex(0));
	TestTrue("Last index should be valid", Array.IsValidIndex(2));
	TestFalse("Past-end index should be invalid", Array.IsValidIndex(3));
	TestFalse("Negative index should be invalid", Array.IsValidIndex(-1));
	TestEqual("operator[] should return mutable element", Array[1], 20);
	TestEqual("const operator[] should return const element", ConstArray[2], 30);
	TestEqual("Last should return final element", Array.Last(), 30);

	Array[1] = 200;
	TestEqual("operator[] should allow mutation", Array[1], 200);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayInvalidIndexDeathTest,
	"System.Core.Containers.Array.InvalidIndexDeathTest")
{
	TArray<int32_t> Array{1, 2};

	ENIGMA_EXPECT_FATAL_ASSERT(
		(void)Array[2],
		"Array index out of bounds: 2 into an array of size 2");

	ENIGMA_EXPECT_FATAL_ASSERT(
		(void)Array[-1],
		"Array index out of bounds: -1 into an array of size 2");
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayInvalidRangeDeathTest,
	"System.Core.Containers.Array.InvalidRangeDeathTest")
{
	TArray<int32_t> Array{1, 2};

	ENIGMA_EXPECT_FATAL_ASSERT(
		Array.RemoveAt(1, 2),
		"Array range out of bounds: index 1 and length 2 into an array of size 2");

	ENIGMA_EXPECT_FATAL_ASSERT(
		Array.RemoveAtSwap(-1, 1),
		"Array range out of bounds: index -1 and length 1 into an array of size 2");
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayLastOnEmptyDeathTest,
	"System.Core.Containers.Array.LastOnEmptyDeathTest")
{
	TArray<int32_t> Array;

	ENIGMA_EXPECT_FATAL_ASSERT(
		(void)Array.Last(),
		"Array index out of bounds: -1 into an array of size 0");
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayAddEmplaceAppendTest,
	"System.Core.Containers.Array.AddEmplaceAppend")
{
	TArray<int32_t> Array;
	const int32_t LValue = 20;
	TArray<int32_t> Other{50, 60};

	TestEqual("Add should return first inserted index", Array.Add(10), 0);
	TestEqual("Add lvalue should return inserted index", Array.Add(LValue), 1);
	TestEqual("Emplace should return inserted index", Array.Emplace(30), 2);
	Array.Append({40, 40});
	Array.Append(Other);

	TestEqual("Append should update count", Array.Num(), 7);
	TestEqual("First appended duplicate should be preserved", Array[3], 40);
	TestEqual("Second appended duplicate should be preserved", Array[4], 40);
	TestEqual("Appended array first value should be preserved", Array[5], 50);
	TestEqual("Appended array second value should be preserved", Array[6], 60);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayPopTest,
	"System.Core.Containers.Array.Pop")
{
	TArray<int32_t> Array{1, 2, 3};

	const int32_t Popped = Array.Pop();

	TestEqual("Pop should return final element", Popped, 3);
	TestEqual("Pop should shrink count", Array.Num(), 2);
	TestEqual("Pop should expose previous element as Last", Array.Last(), 2);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayRemoveAtPreservesOrderTest,
	"System.Core.Containers.Array.RemoveAtPreservesOrder")
{
	TArray<int32_t> Array{1, 2, 2, 3, 4};

	Array.RemoveAt(1, 2);

	TestEqual("RemoveAt should remove requested count", Array.Num(), 3);
	TestEqual("RemoveAt should preserve first element", Array[0], 1);
	TestEqual("RemoveAt should shift first trailing element", Array[1], 3);
	TestEqual("RemoveAt should shift second trailing element", Array[2], 4);

	Array.RemoveAt(Array.Num(), 0);
	TestEqual("Removing zero elements at end should be a no-op", Array.Num(), 3);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayRemoveAtSwapDoesNotPreserveOrderTest,
	"System.Core.Containers.Array.RemoveAtSwapDoesNotPreserveOrder")
{
	TArray<int32_t> Array{1, 2, 3, 4, 5};

	Array.RemoveAtSwap(1, 2);

	TestEqual("RemoveAtSwap should remove requested count", Array.Num(), 3);
	TestEqual("RemoveAtSwap should preserve prefix before removed range", Array[0], 1);
	TestEqual("RemoveAtSwap should move first tail element into gap", Array[1], 4);
	TestEqual("RemoveAtSwap should move second tail element into gap", Array[2], 5);

	Array.RemoveAtSwap(2);
	TestEqual("RemoveAtSwap should remove tail element", Array.Num(), 2);
	TestEqual("Remaining first value should be preserved", Array[0], 1);
	TestEqual("Remaining second value should be preserved", Array[1], 4);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayPointerCompatibleIterationTest,
	"System.Core.Containers.Array.PointerCompatibleIteration")
{
	TArray<int32_t> Array{1, 2, 3};
	const TArray<int32_t>& ConstArray = Array;

	TestTrue("begin should be the data pointer", Array.begin() == Array.GetData());
	TestTrue("const begin should be the const data pointer", ConstArray.begin() == ConstArray.GetData());
	TestTrue("end should be one past the final element", Array.end() == Array.GetData() + Array.Num());

	int32_t Sum = 0;
	for (int32_t Value : Array)
	{
		Sum += Value;
	}
	TestEqual("Range-for should visit all elements", Sum, 6);

	int32_t ReverseDigits = 0;
	for (auto It = Array.rbegin(); It != Array.rend(); ++It)
	{
		ReverseDigits = ReverseDigits * 10 + *It;
	}
	TestEqual("Reverse iteration should walk from tail to head", ReverseDigits, 321);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayMoveOnlyElementsTest,
	"System.Core.Containers.Array.MoveOnlyElements")
{
	TArray<FMoveOnlyValue> Array;
	FMoveOnlyValue Value(2);

	TestEqual("Emplace should return first move-only index", Array.Emplace(1), 0);
	TestEqual("Add rvalue should return second move-only index", Array.Add(std::move(Value)), 1);
	TestEqual("Emplace should return third move-only index", Array.Emplace(3), 2);
	TestEqual("Move-only first value should be stored", Array[0].Value, 1);
	TestEqual("Move-only second value should be stored", Array[1].Value, 2);

	Array.RemoveAt(0);
	TestEqual("RemoveAt should shift move-only values", Array[0].Value, 2);
	TestEqual("RemoveAt should keep trailing move-only value", Array[1].Value, 3);

	FMoveOnlyValue Popped = Array.Pop();
	TestEqual("Pop should return move-only value", Popped.Value, 3);
	TestEqual("Pop should shrink move-only array", Array.Num(), 1);

	Array.RemoveAtSwap(0);
	TestTrue("RemoveAtSwap should clear final move-only value", Array.IsEmpty());
}

ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST(
	FArrayCapacityInvalidationExpectationsTest,
	"System.Core.Containers.Array.CapacityInvalidationExpectations")
{
	TArray<int32_t> Array{1, 2};
	Array.Reserve(4);
	int32_t* StableData = Array.GetData();

	Array.Add(3);

	TestTrue("Add without reallocation should keep storage pointer stable", Array.GetData() == StableData);
	TestEqual("Add without reallocation should preserve capacity", Array.Max(), 4);

	Array.Reserve(16);

	TestTrue("Reserve with capacity growth should allocate storage", Array.GetData() != nullptr);
	TestTrue("Reserve with capacity growth should move away from old storage", Array.GetData() != StableData);
	TestEqual("Reserve with capacity growth should preserve elements", Array[2], 3);
}

#undef ENIGMA_IMPLEMENT_CORE_ARRAY_AUTOMATION_TEST

} // namespace Enigma
