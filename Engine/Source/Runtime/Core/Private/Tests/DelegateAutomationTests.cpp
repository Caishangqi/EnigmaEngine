// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTest.h"
#include "Delegates/Delegate.h"
#include "Delegates/DelegateHandle.h"
#include "Delegates/MulticastDelegate.h"

#include <string>
#include <utility>
#include <vector>

namespace Enigma
{

namespace
{

int StaticAdd(int Left, int Right)
{
	return Left + Right;
}

class FSinglecastTestObject
{
public:
	int Multiply(int Left, int Right)
	{
		return Left * Right;
	}

	void Increment()
	{
		++Counter;
	}

	int Counter = 0;
};

class FMulticastTestListener
{
public:
	void OnEvent(int Value)
	{
		Values.push_back(Value);
	}

	void OnSimple()
	{
		++CallCount;
	}

	std::vector<int> Values;
	int CallCount = 0;
};

} // namespace

#define ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(TestClass, PrettyName) \
	ENIGMA_IMPLEMENT_SIMPLE_AUTOMATION_TEST(                              \
		TestClass,                                                         \
		PrettyName,                                                        \
		Core,                                                              \
		EAutomationTestType::Unit,                                         \
		EAutomationTestFlags::None)                                        \
	bool TestClass::RunTest(const FAutomationTestContext& Context)

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateHandleDefaultConstructedIsInvalidTest,
	"System.Core.Delegates.Handle.DefaultConstructedIsInvalid")
{
	const FDelegateHandle Handle;
	TestTrue("Default delegate handle should be invalid", !Handle.IsValid());
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateHandleGenerateReturnsValidHandleTest,
	"System.Core.Delegates.Handle.GenerateReturnsValidHandle")
{
	const FDelegateHandle Handle = FDelegateHandle::Generate();
	TestTrue("Generated delegate handle should be valid", Handle.IsValid());
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateHandleGenerateReturnsUniqueIdsTest,
	"System.Core.Delegates.Handle.GenerateReturnsUniqueIds")
{
	constexpr int Count = 100;
	std::vector<FDelegateHandle> Handles;
	Handles.reserve(Count);

	for (int Index = 0; Index < Count; ++Index)
	{
		Handles.push_back(FDelegateHandle::Generate());
	}

	for (int LeftIndex = 0; LeftIndex < Count; ++LeftIndex)
	{
		for (int RightIndex = LeftIndex + 1; RightIndex < Count; ++RightIndex)
		{
			TestTrue("Generated delegate handles should be unique", Handles[LeftIndex] != Handles[RightIndex]);
		}
	}

	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateHandleResetMakesInvalidTest,
	"System.Core.Delegates.Handle.ResetMakesInvalid")
{
	FDelegateHandle Handle = FDelegateHandle::Generate();
	TestTrue("Generated delegate handle should start valid", Handle.IsValid());

	Handle.Reset();
	TestTrue("Reset delegate handle should be invalid", !Handle.IsValid());
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateHandleEqualityOperatorTest,
	"System.Core.Delegates.Handle.EqualityOperator")
{
	const FDelegateHandle First = FDelegateHandle::Generate();
	const FDelegateHandle Second = First;

	TestTrue("Copied delegate handle should compare equal", First == Second);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateHandleInequalityOperatorTest,
	"System.Core.Delegates.Handle.InequalityOperator")
{
	const FDelegateHandle First = FDelegateHandle::Generate();
	const FDelegateHandle Second = FDelegateHandle::Generate();

	TestTrue("Different generated delegate handles should not compare equal", First != Second);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateHandleDefaultConstructedHandlesAreEqualTest,
	"System.Core.Delegates.Handle.DefaultConstructedHandlesAreEqual")
{
	const FDelegateHandle First;
	const FDelegateHandle Second;

	TestTrue("Default delegate handles should compare equal", First == Second);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateHandleCopyPreservesValidityTest,
	"System.Core.Delegates.Handle.CopyPreservesValidity")
{
	const FDelegateHandle Original = FDelegateHandle::Generate();
	const FDelegateHandle Copy = Original;

	TestTrue("Copied delegate handle should be valid", Copy.IsValid());
	TestTrue("Copied delegate handle should compare equal to original", Original == Copy);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateHandleMoveTransfersStateTest,
	"System.Core.Delegates.Handle.MoveTransfersState")
{
	FDelegateHandle Original = FDelegateHandle::Generate();
	const FDelegateHandle Moved = std::move(Original);

	TestTrue("Moved delegate handle should be valid", Moved.IsValid());
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateDefaultConstructedIsUnboundTest,
	"System.Core.Delegates.Singlecast.DefaultConstructedIsUnbound")
{
	const TDelegate<int(int, int)> Delegate;
	TestTrue("Default delegate should not be bound", !Delegate.IsBound());
	TestTrue("Default delegate bool conversion should be false", !static_cast<bool>(Delegate));
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateBindLambdaAndExecuteTest,
	"System.Core.Delegates.Singlecast.BindLambdaAndExecute")
{
	TDelegate<int(int, int)> Delegate;
	Delegate.Bind([](int Left, int Right) { return Left + Right; });

	TestTrue("Lambda-bound delegate should be bound", Delegate.IsBound());
	TestEqual("Lambda-bound delegate should execute", Delegate.Execute(3, 4), 7);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateBindStaticFunctionAndExecuteTest,
	"System.Core.Delegates.Singlecast.BindStaticFunctionAndExecute")
{
	TDelegate<int(int, int)> Delegate;
	Delegate.Bind(&StaticAdd);

	TestTrue("Static function delegate should be bound", Delegate.IsBound());
	TestEqual("Static function delegate should execute", Delegate.Execute(5, 6), 11);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateBindMemberFunctionAndExecuteTest,
	"System.Core.Delegates.Singlecast.BindMemberFunctionAndExecute")
{
	FSinglecastTestObject Object;
	TDelegate<int(int, int)> Delegate;
	Delegate.Bind(&Object, &FSinglecastTestObject::Multiply);

	TestTrue("Member function delegate should be bound", Delegate.IsBound());
	TestEqual("Member function delegate should execute", Delegate.Execute(3, 7), 21);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateUnboundExecuteReturnsDefaultTest,
	"System.Core.Delegates.Singlecast.UnboundExecuteReturnsDefault")
{
	TDelegate<int()> IntDelegate;
	TDelegate<float()> FloatDelegate;
	TDelegate<std::string()> StringDelegate;

	TestEqual("Unbound int delegate should return default value", IntDelegate.Execute(), 0);
	TestEqual("Unbound float delegate should return default value", FloatDelegate.Execute(), 0.0f);
	TestEqual("Unbound string delegate should return default value", StringDelegate.Execute(), std::string());
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateUnboundVoidExecuteIsNoOpTest,
	"System.Core.Delegates.Singlecast.UnboundVoidExecuteIsNoOp")
{
	TDelegate<void()> Delegate;
	Delegate.Execute();
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateExecuteIfBoundReturnsTrueWhenBoundTest,
	"System.Core.Delegates.Singlecast.ExecuteIfBoundReturnsTrueWhenBound")
{
	TDelegate<void()> Delegate;
	int CallCount = 0;
	Delegate.Bind([&CallCount]() { ++CallCount; });

	TestTrue("ExecuteIfBound should return true when bound", Delegate.ExecuteIfBound());
	TestEqual("ExecuteIfBound should execute bound delegate", CallCount, 1);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateExecuteIfBoundReturnsFalseWhenUnboundTest,
	"System.Core.Delegates.Singlecast.ExecuteIfBoundReturnsFalseWhenUnbound")
{
	TDelegate<void()> Delegate;
	TestTrue("ExecuteIfBound should return false when unbound", !Delegate.ExecuteIfBound());
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateUnbindClearsBindingTest,
	"System.Core.Delegates.Singlecast.UnbindClearsBinding")
{
	TDelegate<int()> Delegate;
	Delegate.Bind([]() { return 42; });
	TestTrue("Delegate should start bound", Delegate.IsBound());

	Delegate.Unbind();
	TestTrue("Unbound delegate should report unbound", !Delegate.IsBound());
	TestEqual("Unbound delegate should return default value", Delegate.Execute(), 0);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateRebindOverwritesPreviousTest,
	"System.Core.Delegates.Singlecast.RebindOverwritesPrevious")
{
	TDelegate<int()> Delegate;
	Delegate.Bind([]() { return 1; });
	TestEqual("Initial binding should execute", Delegate.Execute(), 1);

	Delegate.Bind([]() { return 2; });
	TestEqual("Second binding should replace first binding", Delegate.Execute(), 2);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateMoveTransfersBindingTest,
	"System.Core.Delegates.Singlecast.MoveTransfersBinding")
{
	TDelegate<int()> Delegate;
	Delegate.Bind([]() { return 99; });

	TDelegate<int()> Moved = std::move(Delegate);
	TestTrue("Moved delegate should remain bound", Moved.IsBound());
	TestEqual("Moved delegate should execute transferred binding", Moved.Execute(), 99);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateCopyPreservesBindingTest,
	"System.Core.Delegates.Singlecast.CopyPreservesBinding")
{
	TDelegate<int()> Delegate;
	Delegate.Bind([]() { return 55; });

	TDelegate<int()> Copy = Delegate;
	TestTrue("Copied delegate should be bound", Copy.IsBound());
	TestEqual("Copied delegate should execute copied binding", Copy.Execute(), 55);
	TestEqual("Original delegate should still execute", Delegate.Execute(), 55);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateVoidMemberFunctionTest,
	"System.Core.Delegates.Singlecast.VoidMemberFunction")
{
	FSinglecastTestObject Object;
	TDelegate<void()> Delegate;
	Delegate.Bind(&Object, &FSinglecastTestObject::Increment);

	Delegate.Execute();
	TestEqual("Void member delegate should execute once", Object.Counter, 1);

	Delegate.Execute();
	TestEqual("Void member delegate should execute twice", Object.Counter, 2);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FDelegateOperatorBoolMatchesIsBoundTest,
	"System.Core.Delegates.Singlecast.OperatorBoolMatchesIsBound")
{
	TDelegate<void()> Delegate;
	TestTrue("Unbound delegate bool conversion should be false", !static_cast<bool>(Delegate));

	Delegate.Bind([]() {});
	TestTrue("Bound delegate bool conversion should be true", static_cast<bool>(Delegate));

	Delegate.Unbind();
	TestTrue("Unbound delegate bool conversion should return to false", !static_cast<bool>(Delegate));
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateDefaultConstructedIsEmptyTest,
	"System.Core.Delegates.Multicast.DefaultConstructedIsEmpty")
{
	const TMulticastDelegate<int> Delegate;
	TestTrue("Default multicast delegate should not be bound", !Delegate.IsBound());
	TestEqual("Default multicast delegate should have no listeners", Delegate.GetCount(), 0);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateAddReturnsValidHandleTest,
	"System.Core.Delegates.Multicast.AddReturnsValidHandle")
{
	TMulticastDelegate<> Delegate;
	const FDelegateHandle Handle = Delegate.Add([]() {});

	TestTrue("Added listener handle should be valid", Handle.IsValid());
	TestEqual("Adding one listener should increase count", Delegate.GetCount(), 1);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateBroadcastCallsAllListenersTest,
	"System.Core.Delegates.Multicast.BroadcastCallsAllListeners")
{
	int CallCount = 0;
	TMulticastDelegate<> Delegate;

	Delegate.Add([&CallCount]() { ++CallCount; });
	Delegate.Add([&CallCount]() { ++CallCount; });
	Delegate.Add([&CallCount]() { ++CallCount; });

	Delegate.Broadcast();
	TestEqual("Broadcast should call every listener", CallCount, 3);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateBroadcastCallsInRegistrationOrderTest,
	"System.Core.Delegates.Multicast.BroadcastCallsInRegistrationOrder")
{
	std::vector<int> Order;
	TMulticastDelegate<> Delegate;

	Delegate.Add([&Order]() { Order.push_back(1); });
	Delegate.Add([&Order]() { Order.push_back(2); });
	Delegate.Add([&Order]() { Order.push_back(3); });

	Delegate.Broadcast();

	TestEqual("Broadcast should call three listeners", Order.size(), 3u);
	if (Order.size() == 3u)
	{
		TestEqual("First listener should run first", Order[0], 1);
		TestEqual("Second listener should run second", Order[1], 2);
		TestEqual("Third listener should run third", Order[2], 3);
	}
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateBroadcastPassesArgumentsTest,
	"System.Core.Delegates.Multicast.BroadcastPassesArguments")
{
	int ReceivedValue = 0;
	TMulticastDelegate<int> Delegate;

	Delegate.Add([&ReceivedValue](int Value) { ReceivedValue = Value; });
	Delegate.Broadcast(42);

	TestEqual("Broadcast should pass arguments", ReceivedValue, 42);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateRemoveByHandleStopsBroadcastTest,
	"System.Core.Delegates.Multicast.RemoveByHandleStopsBroadcast")
{
	int CallCount = 0;
	TMulticastDelegate<> Delegate;

	const FDelegateHandle FirstHandle = Delegate.Add([&CallCount]() { ++CallCount; });
	Delegate.Add([&CallCount]() { ++CallCount; });

	TestTrue("Remove should return true for registered handle", Delegate.Remove(FirstHandle));
	TestEqual("Removing one listener should decrease count", Delegate.GetCount(), 1);

	Delegate.Broadcast();
	TestEqual("Removed listener should not be called", CallCount, 1);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateRemoveInvalidHandleReturnsFalseTest,
	"System.Core.Delegates.Multicast.RemoveInvalidHandleReturnsFalse")
{
	TMulticastDelegate<> Delegate;
	Delegate.Add([]() {});

	const FDelegateHandle InvalidHandle;
	const FDelegateHandle UnrelatedHandle = FDelegateHandle::Generate();
	TestTrue("Remove should return false for default handle", !Delegate.Remove(InvalidHandle));
	TestTrue("Remove should return false for unrelated handle", !Delegate.Remove(UnrelatedHandle));
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateRemoveAllByObjectTest,
	"System.Core.Delegates.Multicast.RemoveAllByObject")
{
	FMulticastTestListener ListenerA;
	FMulticastTestListener ListenerB;
	TMulticastDelegate<> Delegate;

	Delegate.Add(&ListenerA, &FMulticastTestListener::OnSimple);
	Delegate.Add(&ListenerA, &FMulticastTestListener::OnSimple);
	Delegate.Add(&ListenerB, &FMulticastTestListener::OnSimple);

	TestEqual("Initial listener count should be three", Delegate.GetCount(), 3);

	Delegate.RemoveAll(&ListenerA);
	TestEqual("RemoveAll should remove listeners for one object", Delegate.GetCount(), 1);

	Delegate.Broadcast();
	TestEqual("Removed object listeners should not be called", ListenerA.CallCount, 0);
	TestEqual("Remaining object listener should be called", ListenerB.CallCount, 1);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateClearRemovesAllTest,
	"System.Core.Delegates.Multicast.ClearRemovesAll")
{
	TMulticastDelegate<> Delegate;
	Delegate.Add([]() {});
	Delegate.Add([]() {});
	Delegate.Add([]() {});

	TestEqual("Initial listener count should be three", Delegate.GetCount(), 3);

	Delegate.Clear();
	TestEqual("Clear should remove every listener", Delegate.GetCount(), 0);
	TestTrue("Cleared multicast delegate should not be bound", !Delegate.IsBound());
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateBroadcastDuringModifySafeTest,
	"System.Core.Delegates.Multicast.BroadcastDuringModifySafe")
{
	TMulticastDelegate<> Delegate;
	FDelegateHandle SelfHandle;
	int CallCount = 0;

	SelfHandle = Delegate.Add([&]()
	{
		++CallCount;
		Delegate.Remove(SelfHandle);
	});
	Delegate.Add([&CallCount]() { ++CallCount; });

	Delegate.Broadcast();
	TestEqual("Broadcast should call listeners from an iteration copy", CallCount, 2);
	TestEqual("Self-removal should affect future broadcasts", Delegate.GetCount(), 1);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateBroadcastDuringAddSafeTest,
	"System.Core.Delegates.Multicast.BroadcastDuringAddSafe")
{
	TMulticastDelegate<> Delegate;
	int CallCount = 0;

	Delegate.Add([&]()
	{
		++CallCount;
		Delegate.Add([&CallCount]() { ++CallCount; });
	});

	Delegate.Broadcast();
	TestEqual("New listener should not run during current broadcast", CallCount, 1);
	TestEqual("Adding during broadcast should add future listener", Delegate.GetCount(), 2);

	CallCount = 0;
	Delegate.Broadcast();
	TestEqual("Second broadcast should call both listeners", CallCount, 2);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateMemberFunctionBindingTest,
	"System.Core.Delegates.Multicast.MemberFunctionBinding")
{
	FMulticastTestListener Listener;
	TMulticastDelegate<int> Delegate;

	Delegate.Add(&Listener, &FMulticastTestListener::OnEvent);
	Delegate.Broadcast(10);
	Delegate.Broadcast(20);

	TestEqual("Member listener should receive two values", Listener.Values.size(), 2u);
	if (Listener.Values.size() == 2u)
	{
		TestEqual("First broadcast value should be received", Listener.Values[0], 10);
		TestEqual("Second broadcast value should be received", Listener.Values[1], 20);
	}
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateMoveSemanticsTest,
	"System.Core.Delegates.Multicast.MoveSemantics")
{
	int CallCount = 0;
	TMulticastDelegate<> Delegate;
	Delegate.Add([&CallCount]() { ++CallCount; });

	TMulticastDelegate<> Moved = std::move(Delegate);
	TestTrue("Moved multicast delegate should remain bound", Moved.IsBound());

	Moved.Broadcast();
	TestEqual("Moved multicast delegate should execute listener", CallCount, 1);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST(
	FMulticastDelegateMultipleArgumentsTest,
	"System.Core.Delegates.Multicast.MultipleArguments")
{
	float ResultA = 0.0f;
	float ResultB = 0.0f;
	TMulticastDelegate<float, float> Delegate;

	Delegate.Add([&ResultA](float Left, float Right) { ResultA = Left + Right; });
	Delegate.Add([&ResultB](float Left, float Right) { ResultB = Left * Right; });

	Delegate.Broadcast(3.0f, 4.0f);
	TestEqual("First listener should receive both arguments", ResultA, 7.0f);
	TestEqual("Second listener should receive both arguments", ResultB, 12.0f);
	return !Context.HasAnyFailures();
}

#undef ENIGMA_IMPLEMENT_DELEGATE_AUTOMATION_TEST

} // namespace Enigma
