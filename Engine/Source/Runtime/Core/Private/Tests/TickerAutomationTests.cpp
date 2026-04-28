// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTest.h"
#include "Containers/Ticker.h"

namespace Enigma
{

namespace
{

class FTickerAutomationTestScope
{
public:
	FTickerAutomationTestScope()
	{
		GetTicker().Reset();
	}

	~FTickerAutomationTestScope()
	{
		GetTicker().Reset();
	}

	FTSTicker& GetTicker() const
	{
		return FTSTicker::GetCoreTicker();
	}
};

class FTestTickerObject : public FTSTickerObjectBase
{
public:
	explicit FTestTickerObject(float Delay = 0.0f)
		: FTSTickerObjectBase(Delay)
	{
	}

	bool Tick(float DeltaTime) override
	{
		(void)DeltaTime;
		++TickCount;
		return true;
	}

	int TickCount = 0;
};

} // namespace

#define ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST(TestClass, PrettyName) \
	ENIGMA_IMPLEMENT_SIMPLE_AUTOMATION_TEST(                            \
		TestClass,                                                       \
		PrettyName,                                                      \
		Core,                                                            \
		EAutomationTestType::Unit,                                       \
		EAutomationTestFlags::None)                                      \
	bool TestClass::RunTest(const FAutomationTestContext& Context)

ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST(
	FTickerAddTickerOneShotTest,
	"System.Core.Ticker.AddTicker.OneShot")
{
	FTickerAutomationTestScope Scope;
	int CallCount = 0;

	FTickerDelegate Delegate;
	Delegate.Bind([&CallCount](float)
	{
		++CallCount;
		return false;
	});

	Scope.GetTicker().AddTicker(Delegate);

	Scope.GetTicker().Tick(0.016f);
	TestEqual("One-shot ticker should fire once", CallCount, 1);

	Scope.GetTicker().Tick(0.016f);
	TestEqual("One-shot ticker should not fire again", CallCount, 1);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST(
	FTickerAddTickerRecurringTest,
	"System.Core.Ticker.AddTicker.Recurring")
{
	FTickerAutomationTestScope Scope;
	int CallCount = 0;

	FTickerDelegate Delegate;
	Delegate.Bind([&CallCount](float)
	{
		++CallCount;
		return true;
	});

	Scope.GetTicker().AddTicker(Delegate);

	Scope.GetTicker().Tick(0.016f);
	Scope.GetTicker().Tick(0.016f);
	Scope.GetTicker().Tick(0.016f);
	TestEqual("Recurring ticker should fire every tick", CallCount, 3);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST(
	FTickerAddTickerWithDelayTest,
	"System.Core.Ticker.AddTicker.WithDelay")
{
	FTickerAutomationTestScope Scope;
	int CallCount = 0;

	FTickerDelegate Delegate;
	Delegate.Bind([&CallCount](float)
	{
		++CallCount;
		return false;
	});

	Scope.GetTicker().AddTicker(Delegate, 0.5f);

	Scope.GetTicker().Tick(0.1f);
	TestEqual("Delayed ticker should not fire before delay", CallCount, 0);

	Scope.GetTicker().Tick(0.1f);
	TestEqual("Delayed ticker should still wait before delay", CallCount, 0);

	Scope.GetTicker().Tick(0.4f);
	TestEqual("Delayed ticker should fire after delay", CallCount, 1);

	Scope.GetTicker().Tick(1.0f);
	TestEqual("Delayed one-shot ticker should not fire again", CallCount, 1);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST(
	FTickerAddTickerRecurringWithDelayTest,
	"System.Core.Ticker.AddTicker.RecurringWithDelay")
{
	FTickerAutomationTestScope Scope;
	int CallCount = 0;

	FTickerDelegate Delegate;
	Delegate.Bind([&CallCount](float)
	{
		++CallCount;
		return true;
	});

	Scope.GetTicker().AddTicker(Delegate, 1.0f);

	Scope.GetTicker().Tick(0.5f);
	TestEqual("Recurring delayed ticker should wait for delay", CallCount, 0);

	Scope.GetTicker().Tick(0.6f);
	TestEqual("Recurring delayed ticker should fire after delay", CallCount, 1);

	Scope.GetTicker().Tick(0.5f);
	TestEqual("Recurring delayed ticker should reschedule at delay", CallCount, 1);

	Scope.GetTicker().Tick(0.6f);
	TestEqual("Recurring delayed ticker should fire after rescheduled delay", CallCount, 2);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST(
	FTickerRemoveTickerTest,
	"System.Core.Ticker.RemoveTicker")
{
	FTickerAutomationTestScope Scope;
	int CallCount = 0;

	FTickerDelegate Delegate;
	Delegate.Bind([&CallCount](float)
	{
		++CallCount;
		return true;
	});

	const FDelegateHandle Handle = Scope.GetTicker().AddTicker(Delegate);

	Scope.GetTicker().Tick(0.016f);
	TestEqual("Ticker should fire before removal", CallCount, 1);

	FTSTicker::RemoveTicker(Handle);

	Scope.GetTicker().Tick(0.016f);
	TestEqual("Removed ticker should not fire", CallCount, 1);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST(
	FTickerRemoveTickerBeforeFirstTickTest,
	"System.Core.Ticker.RemoveTicker.BeforeFirstTick")
{
	FTickerAutomationTestScope Scope;
	int CallCount = 0;

	FTickerDelegate Delegate;
	Delegate.Bind([&CallCount](float)
	{
		++CallCount;
		return true;
	});

	const FDelegateHandle Handle = Scope.GetTicker().AddTicker(Delegate);
	FTSTicker::RemoveTicker(Handle);

	Scope.GetTicker().Tick(0.016f);
	TestEqual("Ticker removed before first tick should not fire", CallCount, 0);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST(
	FTickerTickMultipleElementsTest,
	"System.Core.Ticker.Tick.MultipleElements")
{
	FTickerAutomationTestScope Scope;
	int CountA = 0;
	int CountB = 0;
	int CountC = 0;

	FTickerDelegate DelegateA;
	DelegateA.Bind([&CountA](float)
	{
		++CountA;
		return true;
	});

	FTickerDelegate DelegateB;
	DelegateB.Bind([&CountB](float)
	{
		++CountB;
		return false;
	});

	FTickerDelegate DelegateC;
	DelegateC.Bind([&CountC](float)
	{
		++CountC;
		return true;
	});

	Scope.GetTicker().AddTicker(DelegateA);
	Scope.GetTicker().AddTicker(DelegateB);
	Scope.GetTicker().AddTicker(DelegateC);

	Scope.GetTicker().Tick(0.016f);
	TestEqual("First recurring ticker should fire on first tick", CountA, 1);
	TestEqual("One-shot ticker should fire on first tick", CountB, 1);
	TestEqual("Second recurring ticker should fire on first tick", CountC, 1);

	Scope.GetTicker().Tick(0.016f);
	TestEqual("First recurring ticker should fire on second tick", CountA, 2);
	TestEqual("One-shot ticker should not fire on second tick", CountB, 1);
	TestEqual("Second recurring ticker should fire on second tick", CountC, 2);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST(
	FTickerObjectBaseAutoRegisterTest,
	"System.Core.Ticker.ObjectBase.AutoRegister")
{
	FTickerAutomationTestScope Scope;

	{
		FTestTickerObject Object;

		Scope.GetTicker().Tick(0.016f);
		TestEqual("Ticker object should auto-register", Object.TickCount, 1);

		Scope.GetTicker().Tick(0.016f);
		TestEqual("Ticker object should keep ticking", Object.TickCount, 2);
	}

	Scope.GetTicker().Tick(0.016f);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST(
	FTickerObjectBaseWithDelayTest,
	"System.Core.Ticker.ObjectBase.WithDelay")
{
	FTickerAutomationTestScope Scope;
	FTestTickerObject Object(0.5f);

	Scope.GetTicker().Tick(0.1f);
	TestEqual("Delayed ticker object should wait for delay", Object.TickCount, 0);

	Scope.GetTicker().Tick(0.5f);
	TestEqual("Delayed ticker object should tick after delay", Object.TickCount, 1);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST(
	FTickerDeltaTimePassedToDelegateTest,
	"System.Core.Ticker.DeltaTime.PassedToDelegate")
{
	FTickerAutomationTestScope Scope;
	float ReceivedDeltaTime = 0.0f;

	FTickerDelegate Delegate;
	Delegate.Bind([&ReceivedDeltaTime](float DeltaTime)
	{
		ReceivedDeltaTime = DeltaTime;
		return false;
	});

	Scope.GetTicker().AddTicker(Delegate);
	Scope.GetTicker().Tick(0.033f);
	TestEqual("Ticker should pass delta time to delegate", ReceivedDeltaTime, 0.033f);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST(
	FTickerResetClearsAllTest,
	"System.Core.Ticker.Reset.ClearsAll")
{
	FTickerAutomationTestScope Scope;
	int CallCount = 0;

	FTickerDelegate Delegate;
	Delegate.Bind([&CallCount](float)
	{
		++CallCount;
		return true;
	});

	Scope.GetTicker().AddTicker(Delegate);
	Scope.GetTicker().Tick(0.016f);
	TestEqual("Ticker should fire before reset", CallCount, 1);

	Scope.GetTicker().Reset();

	Scope.GetTicker().Tick(0.016f);
	TestEqual("Ticker should not fire after reset", CallCount, 1);
	return !Context.HasAnyFailures();
}

#undef ENIGMA_IMPLEMENT_TICKER_AUTOMATION_TEST

} // namespace Enigma
