// Copyright EnigmaEngine. All Rights Reserved.

#include "Algo/Find.h"
#include "AutomationTest/AutomationTest.h"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"

#include <cstdint>

namespace Enigma
{

namespace
{

struct FSearchRecord
{
	FSearchRecord(int32_t InId, int32_t InScore)
		: Id(InId)
		, Score(InScore)
	{
	}

	int32_t Id = 0;
	int32_t Score = 0;
};

} // namespace

#define ENIGMA_IMPLEMENT_CORE_ALGO_FIND_AUTOMATION_TEST(TestClass, PrettyName) \
	ENIGMA_IMPLEMENT_AUTOMATION_TEST(                                           \
		TestClass,                                                              \
		PrettyName,                                                             \
		Core,                                                                   \
		EAutomationTestType::Unit,                                              \
		EAutomationTestFlags::None)

ENIGMA_IMPLEMENT_CORE_ALGO_FIND_AUTOMATION_TEST(
	FAlgoFindEmptyRangeTest,
	"System.Core.Algo.Find.EmptyRange")
{
	TArray<int32_t> Array;

	TestTrue("Find on empty range should return null", Algo::Find(Array, 1) == nullptr);
	TestFalse("Contains on empty range should be false", Algo::Contains(Array, 1));
	TestEqual("IndexOf on empty range should return -1", Algo::IndexOf(Array, 1), -1);
}

ENIGMA_IMPLEMENT_CORE_ALGO_FIND_AUTOMATION_TEST(
	FAlgoFindSingleElementTest,
	"System.Core.Algo.Find.SingleElement")
{
	TArray<int32_t> Array{42};

	int32_t* Found = Algo::Find(Array, 42);

	TestTrue("Find should return a pointer for the matching element", Found == &Array[0]);
	TestTrue("Contains should be true for the matching element", Algo::Contains(Array, 42));
	TestEqual("IndexOf should return zero for the first element", Algo::IndexOf(Array, 42), 0);
}

ENIGMA_IMPLEMENT_CORE_ALGO_FIND_AUTOMATION_TEST(
	FAlgoFindDuplicateReturnsFirstTest,
	"System.Core.Algo.Find.DuplicateReturnsFirst")
{
	TArray<int32_t> Array{5, 7, 7, 9};

	int32_t* Found = Algo::Find(Array, 7);

	TestTrue("Find should return first duplicate", Found == &Array[1]);
	TestEqual("IndexOf should return first duplicate index", Algo::IndexOf(Array, 7), 1);
}

ENIGMA_IMPLEMENT_CORE_ALGO_FIND_AUTOMATION_TEST(
	FAlgoFindNotFoundTest,
	"System.Core.Algo.Find.NotFound")
{
	TArray<int32_t> Array{1, 2, 3};

	TestTrue("Find should return null when value is missing", Algo::Find(Array, 99) == nullptr);
	TestFalse("Contains should be false when value is missing", Algo::Contains(Array, 99));
	TestEqual("IndexOf should return -1 when value is missing", Algo::IndexOf(Array, 99), -1);
}

ENIGMA_IMPLEMENT_CORE_ALGO_FIND_AUTOMATION_TEST(
	FAlgoFindConstRangeTest,
	"System.Core.Algo.Find.ConstRange")
{
	const TArray<int32_t> Array{2, 4, 6};

	const int32_t* Found = Algo::Find(Array, 4);

	TestTrue("Find on const range should return const pointer", Found == &Array[1]);
	TestEqual("IndexOf on const range should find value", Algo::IndexOf(Array, 6), 2);
}

ENIGMA_IMPLEMENT_CORE_ALGO_FIND_AUTOMATION_TEST(
	FAlgoFindArrayViewTest,
	"System.Core.Algo.Find.ArrayView")
{
	TArray<int32_t> Array{10, 20, 30};
	TArrayView<int32_t> View(Array);

	int32_t* Found = Algo::Find(View, 20);

	TestTrue("Find should work with TArrayView", Found == &Array[1]);
	TestTrue("Contains should work with TArrayView", Algo::Contains(View, 30));
	TestEqual("IndexOf should work with TArrayView", Algo::IndexOf(View, 10), 0);
}

ENIGMA_IMPLEMENT_CORE_ALGO_FIND_AUTOMATION_TEST(
	FAlgoFindConstArrayViewTest,
	"System.Core.Algo.Find.ConstArrayView")
{
	TArray<int32_t> Array{11, 22, 33};
	TConstArrayView<int32_t> View(Array);

	const int32_t* Found = Algo::Find(View, 33);

	TestTrue("Find should work with TConstArrayView", Found == &Array[2]);
	TestTrue("Contains should work with TConstArrayView", Algo::Contains(View, 11));
	TestEqual("IndexOf should work with TConstArrayView", Algo::IndexOf(View, 22), 1);
}

ENIGMA_IMPLEMENT_CORE_ALGO_FIND_AUTOMATION_TEST(
	FAlgoFindPredicateTest,
	"System.Core.Algo.Find.Predicate")
{
	TArray<FSearchRecord> Records;
	Records.Emplace(1, 100);
	Records.Emplace(2, 250);
	Records.Emplace(3, 175);

	FSearchRecord* Found = Algo::FindByPredicate(Records, [](const FSearchRecord& Record)
	{
		return Record.Score > 200;
	});

	TestTrue("FindByPredicate should return matching record", Found == &Records[1]);
	TestTrue("ContainsByPredicate should find matching record", Algo::ContainsByPredicate(Records, [](const FSearchRecord& Record)
	{
		return Record.Id == 3;
	}));
	TestEqual("IndexOfByPredicate should return matching index", Algo::IndexOfByPredicate(Records, [](const FSearchRecord& Record)
	{
		return Record.Score == 175;
	}), 2);
	TestEqual("IndexOfByPredicate should return -1 for missing record", Algo::IndexOfByPredicate(Records, [](const FSearchRecord& Record)
	{
		return Record.Id == 99;
	}), -1);
}

#undef ENIGMA_IMPLEMENT_CORE_ALGO_FIND_AUTOMATION_TEST

} // namespace Enigma
