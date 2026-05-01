// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTest.h"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"

#include <cstdint>
#include <type_traits>

namespace Enigma
{

#define ENIGMA_IMPLEMENT_CORE_ARRAY_VIEW_AUTOMATION_TEST(TestClass, PrettyName) \
	ENIGMA_IMPLEMENT_AUTOMATION_TEST(                                            \
		TestClass,                                                               \
		PrettyName,                                                              \
		Core,                                                                    \
		EAutomationTestType::Unit,                                               \
		EAutomationTestFlags::None)

ENIGMA_IMPLEMENT_CORE_ARRAY_VIEW_AUTOMATION_TEST(
	FArrayViewDefaultConstructionTest,
	"System.Core.Containers.ArrayView.DefaultConstruction")
{
	TArrayView<int32_t> View;

	TestEqual("Default view should have zero elements", View.Num(), 0);
	TestTrue("Default view should be empty", View.IsEmpty());
	TestTrue("Default view should not reference data", View.GetData() == nullptr);
	TestFalse("Default view should not have a valid index", View.IsValidIndex(0));
}

ENIGMA_IMPLEMENT_CORE_ARRAY_VIEW_AUTOMATION_TEST(
	FArrayViewPointerCountMutableTest,
	"System.Core.Containers.ArrayView.PointerCountMutable")
{
	int32_t Values[] = {1, 2, 3};
	TArrayView<int32_t> View(Values, 3);

	TestEqual("Pointer/count view should expose count", View.Num(), 3);
	TestTrue("Pointer/count view should reference original data", View.GetData() == Values);
	TestEqual("Pointer/count view should expose values", View[1], 2);

	View[1] = 20;
	TestEqual("Mutable view should mutate referenced data", Values[1], 20);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_VIEW_AUTOMATION_TEST(
	FArrayViewPointerCountConstTest,
	"System.Core.Containers.ArrayView.PointerCountConst")
{
	const int32_t Values[] = {4, 5, 6};
	TConstArrayView<int32_t> View(Values, 3);

	static_assert(std::is_const_v<std::remove_reference_t<decltype(View[0])>>);
	TestEqual("Const view should expose count", View.Num(), 3);
	TestEqual("Const view should expose values", View[2], 6);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_VIEW_AUTOMATION_TEST(
	FArrayViewCArrayConstructionTest,
	"System.Core.Containers.ArrayView.CArrayConstruction")
{
	int32_t Values[] = {7, 8, 9, 10};
	TArrayView<int32_t> View(Values);

	TestEqual("C array view should infer count", View.Num(), 4);
	TestEqual("C array view should expose first value", View[0], 7);
	TestEqual("C array view should expose last value", View[3], 10);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_VIEW_AUTOMATION_TEST(
	FArrayViewTArrayMutableConstructionTest,
	"System.Core.Containers.ArrayView.TArrayMutableConstruction")
{
	TArray<int32_t> Array{11, 12, 13};
	TArrayView<int32_t> View(Array);

	TestEqual("TArray view should expose count", View.Num(), Array.Num());
	TestTrue("TArray view should reference array storage", View.GetData() == Array.GetData());

	View[0] = 110;
	TestEqual("Mutable TArray view should mutate array storage", Array[0], 110);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_VIEW_AUTOMATION_TEST(
	FArrayViewTArrayConstConstructionTest,
	"System.Core.Containers.ArrayView.TArrayConstConstruction")
{
	TArray<int32_t> Array{14, 15, 16};
	const TArray<int32_t>& ConstArray = Array;
	TConstArrayView<int32_t> View(ConstArray);

	static_assert(std::is_const_v<std::remove_reference_t<decltype(View[0])>>);
	TestEqual("Const TArray view should expose count", View.Num(), 3);
	TestTrue("Const TArray view should reference array storage", View.GetData() == Array.GetData());
	TestEqual("Const TArray view should expose values", View[1], 15);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_VIEW_AUTOMATION_TEST(
	FArrayViewIterationTest,
	"System.Core.Containers.ArrayView.Iteration")
{
	int32_t Values[] = {1, 2, 3};
	TArrayView<int32_t> View(Values);

	TestTrue("View begin should be data pointer", View.begin() == View.GetData());
	TestTrue("View end should be one past final element", View.end() == View.GetData() + View.Num());

	int32_t Sum = 0;
	for (int32_t Value : View)
	{
		Sum += Value;
	}
	TestEqual("View range-for should visit all elements", Sum, 6);

	int32_t ReverseDigits = 0;
	for (auto It = View.rbegin(); It != View.rend(); ++It)
	{
		ReverseDigits = ReverseDigits * 10 + *It;
	}
	TestEqual("View reverse iteration should walk tail to head", ReverseDigits, 321);
}

ENIGMA_IMPLEMENT_CORE_ARRAY_VIEW_AUTOMATION_TEST(
	FArrayViewRecoverableBoundaryChecksTest,
	"System.Core.Containers.ArrayView.RecoverableBoundaryChecks")
{
	int32_t Values[] = {21, 22};
	TArrayView<int32_t> View(Values);

	TestTrue("First index should be valid", View.IsValidIndex(0));
	TestTrue("Last index should be valid", View.IsValidIndex(1));
	TestFalse("Past-end index should be invalid", View.IsValidIndex(2));
	TestFalse("Negative index should be invalid", View.IsValidIndex(-1));
}

ENIGMA_IMPLEMENT_CORE_ARRAY_VIEW_AUTOMATION_TEST(
	FArrayViewInvalidIndexDeathTest,
	"System.Core.Containers.ArrayView.InvalidIndexDeathTest")
{
	int32_t Values[] = {31, 32};
	TArrayView<int32_t> View(Values);

	ENIGMA_EXPECT_FATAL_ASSERT(
		(void)View[2],
		"Array index out of bounds: 2 from an array of size 2");

	ENIGMA_EXPECT_FATAL_ASSERT(
		(void)View[-1],
		"Array index out of bounds: -1 from an array of size 2");
}

#undef ENIGMA_IMPLEMENT_CORE_ARRAY_VIEW_AUTOMATION_TEST

} // namespace Enigma
