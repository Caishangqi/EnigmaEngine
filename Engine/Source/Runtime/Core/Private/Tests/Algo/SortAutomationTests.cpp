// Copyright EnigmaEngine. All Rights Reserved.

#include "Algo/Sort.h"
#include "AutomationTest/AutomationTest.h"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"

#include <cstdint>

namespace Enigma
{

#define ENIGMA_IMPLEMENT_CORE_ALGO_SORT_AUTOMATION_TEST(TestClass, PrettyName) \
	ENIGMA_IMPLEMENT_AUTOMATION_TEST(                                           \
		TestClass,                                                              \
		PrettyName,                                                             \
		Core,                                                                   \
		EAutomationTestType::Unit,                                              \
		EAutomationTestFlags::None)

ENIGMA_IMPLEMENT_CORE_ALGO_SORT_AUTOMATION_TEST(
	FAlgoSortAscendingTest,
	"System.Core.Algo.Sort.Ascending")
{
	TArray<int32_t> Array{4, 1, 3, 2};

	Algo::Sort(Array);

	TestEqual("First sorted value should be lowest", Array[0], 1);
	TestEqual("Second sorted value should be ordered", Array[1], 2);
	TestEqual("Third sorted value should be ordered", Array[2], 3);
	TestEqual("Fourth sorted value should be highest", Array[3], 4);
}

ENIGMA_IMPLEMENT_CORE_ALGO_SORT_AUTOMATION_TEST(
	FAlgoSortCustomComparatorTest,
	"System.Core.Algo.Sort.CustomComparator")
{
	TArray<int32_t> Array{4, 1, 3, 2};

	Algo::Sort(Array, [](int32_t Left, int32_t Right)
	{
		return Left > Right;
	});

	TestEqual("First custom sorted value should be highest", Array[0], 4);
	TestEqual("Second custom sorted value should be ordered", Array[1], 3);
	TestEqual("Third custom sorted value should be ordered", Array[2], 2);
	TestEqual("Fourth custom sorted value should be lowest", Array[3], 1);
}

ENIGMA_IMPLEMENT_CORE_ALGO_SORT_AUTOMATION_TEST(
	FAlgoSortDuplicatesTest,
	"System.Core.Algo.Sort.Duplicates")
{
	TArray<int32_t> Array{3, 1, 2, 1, 3};

	Algo::Sort(Array);

	TestEqual("First duplicate sorted value should be lowest", Array[0], 1);
	TestEqual("Second duplicate sorted value should be lowest", Array[1], 1);
	TestEqual("Middle duplicate sorted value should be ordered", Array[2], 2);
	TestEqual("First high duplicate sorted value should be ordered", Array[3], 3);
	TestEqual("Second high duplicate sorted value should be ordered", Array[4], 3);
}

ENIGMA_IMPLEMENT_CORE_ALGO_SORT_AUTOMATION_TEST(
	FAlgoSortEmptyRangeTest,
	"System.Core.Algo.Sort.EmptyRange")
{
	TArray<int32_t> Array;

	Algo::Sort(Array);

	TestTrue("Sorting an empty range should keep it empty", Array.IsEmpty());
}

ENIGMA_IMPLEMENT_CORE_ALGO_SORT_AUTOMATION_TEST(
	FAlgoSortSingleElementTest,
	"System.Core.Algo.Sort.SingleElement")
{
	TArray<int32_t> Array{42};

	Algo::Sort(Array);

	TestEqual("Sorting a single element should preserve it", Array[0], 42);
}

ENIGMA_IMPLEMENT_CORE_ALGO_SORT_AUTOMATION_TEST(
	FAlgoSortArrayViewTest,
	"System.Core.Algo.Sort.ArrayView")
{
	TArray<int32_t> Array{9, 7, 8};
	TArrayView<int32_t> View(Array);

	Algo::Sort(View);

	TestEqual("ArrayView sort should reorder underlying first value", Array[0], 7);
	TestEqual("ArrayView sort should reorder underlying second value", Array[1], 8);
	TestEqual("ArrayView sort should reorder underlying third value", Array[2], 9);
}

#undef ENIGMA_IMPLEMENT_CORE_ALGO_SORT_AUTOMATION_TEST

} // namespace Enigma
