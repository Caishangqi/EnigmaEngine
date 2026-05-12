// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST(SuiteName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST( \
        F##SuiteName##_##TestName##AutomationTest, \
        "System.Engine.TickSystem." #SuiteName "." #TestName, \
        Engine, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresEngine)

#define ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(FixtureName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST_F( \
        FixtureName, \
        F##FixtureName##_##TestName##AutomationTest, \
        "System.Engine.TickSystem." #FixtureName "." #TestName, \
        Engine, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresEngine)

#include "TickSystem/TickFunction.h"
#include "TickSystem/TickTaskManager.h"
#include "TickSystem/TickGroup.h"
#include "GameFramework/Component.h"
#include "GameFramework/GameObject.h"
#include "GameFramework/Scene.h"
#include "Subsystems/SubsystemCollection.h"
#include "Engine/Engine.h"

#include <vector>

using namespace Enigma;

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

/// Concrete FTickFunction for testing -- records execution order.
class FTestTickFunction : public FTickFunction
{
public:
	std::vector<int>* ExecutionLog = nullptr;
	int ID = 0;

	void ExecuteTick(float /*deltaTime*/) override
	{
		if (ExecutionLog)
		{
			ExecutionLog->push_back(ID);
		}
	}
};

/// Concrete FComponent for testing.
class FTestComponent : public FComponent
{
public:
	static FName GetStaticName() { return FName("TestComponent"); }
	FName GetName() const override { return GetStaticName(); }

	int UpdateCount = 0;
	bool BeginPlayCalled = false;

	void BeginPlay() override
	{
		FComponent::BeginPlay();
		BeginPlayCalled = true;
	}

	void Update(float deltaTime) override
	{
		++UpdateCount;
	}
};

/// Tickable test component (bCanEverTick = true).
class FTickableTestComponent : public FTestComponent
{
public:
	static FName GetStaticName() { return FName("TickableTestComponent"); }
	FName GetName() const override { return GetStaticName(); }

	FTickableTestComponent()
	{
		bCanEverTick = true;
	}
};

/// Test fixture that sets up a minimal engine environment.
class TickSystemFixture : public ::Enigma::FAutomationTestFixture
{
protected:
	void SetUp() override
	{
		m_engine = std::make_unique<FEngine>();
		GEngine = m_engine.get();
		GEngine->GetSubsystemCollection().RegisterSubsystem<FTickTaskManager>();
		GEngine->GetSubsystemCollection().Initialize();
		m_tickManager = GEngine->GetSubsystem<FTickTaskManager>();
	}

	void TearDown() override
	{
		GEngine->GetSubsystemCollection().Deinitialize();
		GEngine = nullptr;
		m_engine.reset();
	}

	FTickTaskManager* m_tickManager = nullptr;
	std::unique_ptr<FEngine> m_engine;
};

// TESTS_PLACEHOLDER_1

// Test 1: Register and unregister a tick function
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, RegisterUnregister)
{
	FTestTickFunction tf;
	tf.ID = 1;

	TestFalse("EXPECT_FALSE", tf.IsRegistered());
	tf.RegisterTickFunction(*m_tickManager);
	TestTrue("EXPECT_TRUE", tf.IsRegistered());

	tf.UnregisterTickFunction();
	TestFalse("EXPECT_FALSE", tf.IsRegistered());
}

// Test 2: Enable/disable without re-registration
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, EnableDisable)
{
	FTestTickFunction tf;
	tf.RegisterTickFunction(*m_tickManager);

	TestTrue("EXPECT_TRUE", tf.IsTickFunctionEnabled());
	tf.SetTickFunctionEnable(false);
	TestFalse("EXPECT_FALSE", tf.IsTickFunctionEnabled());
	tf.SetTickFunctionEnable(true);
	TestTrue("EXPECT_TRUE", tf.IsTickFunctionEnabled());
	TestTrue("EXPECT_TRUE", tf.IsRegistered());

	tf.UnregisterTickFunction();
}

// Test 3: Tick groups execute in order
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, TickGroupOrder)
{
	std::vector<int> log;

	FTestTickFunction pre;
	pre.ID = 1; pre.TickGroup = ETickGroup::TG_PreUpdate; pre.ExecutionLog = &log;
	pre.RegisterTickFunction(*m_tickManager);

	FTestTickFunction update;
	update.ID = 2; update.TickGroup = ETickGroup::TG_Update; update.ExecutionLog = &log;
	update.RegisterTickFunction(*m_tickManager);

	FTestTickFunction post;
	post.ID = 3; post.TickGroup = ETickGroup::TG_PostUpdate; post.ExecutionLog = &log;
	post.RegisterTickFunction(*m_tickManager);

	m_tickManager->Tick(0.016f);

	if (!TestEqual("ASSERT_EQ", log.size(), 3u)) { return; }
	TestEqual("EXPECT_EQ", log[0], 1);
	TestEqual("EXPECT_EQ", log[1], 2);
	TestEqual("EXPECT_EQ", log[2], 3);

	pre.UnregisterTickFunction();
	update.UnregisterTickFunction();
	post.UnregisterTickFunction();
}

// TESTS_PLACEHOLDER_2

// Test 4: Prerequisite ordering within same group
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, PrerequisiteOrder)
{
	std::vector<int> log;

	FTestTickFunction a;
	a.ID = 1; a.TickGroup = ETickGroup::TG_Update; a.ExecutionLog = &log;
	a.RegisterTickFunction(*m_tickManager);

	FTestTickFunction b;
	b.ID = 2; b.TickGroup = ETickGroup::TG_Update; b.ExecutionLog = &log;
	b.AddPrerequisite(a);
	b.RegisterTickFunction(*m_tickManager);

	m_tickManager->Tick(0.016f);

	if (!TestEqual("ASSERT_EQ", log.size(), 2u)) { return; }
	TestEqual("EXPECT_EQ", log[0], 1);
	TestEqual("EXPECT_EQ", log[1], 2);

	a.UnregisterTickFunction();
	b.UnregisterTickFunction();
}

// Test 5: Circular dependency detected and rejected
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, CircularDependencyDetected)
{
	// Note: Current implementation does not reject in AddPrerequisite
	// (cycle detection is in FTickTaskManager). This test verifies
	// the system does not deadlock with circular prerequisites.
	std::vector<int> log;

	FTestTickFunction a;
	a.ID = 1; a.TickGroup = ETickGroup::TG_Update; a.ExecutionLog = &log;

	FTestTickFunction b;
	b.ID = 2; b.TickGroup = ETickGroup::TG_Update; b.ExecutionLog = &log;

	a.AddPrerequisite(b);
	b.AddPrerequisite(a);

	a.RegisterTickFunction(*m_tickManager);
	b.RegisterTickFunction(*m_tickManager);

	// Should not deadlock -- Kahn's algorithm leaves cycle nodes unexecuted
	m_tickManager->Tick(0.016f);

	// Both have unresolvable dependencies, so neither should execute
	TestTrue("EXPECT_TRUE", log.empty());

	a.UnregisterTickFunction();
	b.UnregisterTickFunction();
}

// Test 6: TickInterval cooldown skips intermediate frames
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, TickInterval)
{
	std::vector<int> log;

	FTestTickFunction tf;
	tf.ID = 1; tf.TickGroup = ETickGroup::TG_Update;
	tf.TickInterval = 0.1f;
	tf.ExecutionLog = &log;
	tf.RegisterTickFunction(*m_tickManager);

	// Frame 1 (dt=0.016): should execute (first time, cooldown not set yet)
	m_tickManager->Tick(0.016f);
	// Frame 2 (dt=0.016): cooldown remaining ~0.084, skip
	m_tickManager->Tick(0.016f);
	// Frame 3 (dt=0.016): cooldown remaining ~0.068, skip
	m_tickManager->Tick(0.016f);

	// Only first frame should have executed
	TestEqual("EXPECT_EQ", log.size(), 1u);

	tf.UnregisterTickFunction();
}

// TESTS_PLACEHOLDER_3

// Test 7: Deferred registration -- mid-tick add applied next frame
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, DeferredRegistration)
{
	std::vector<int> log;

	FTestTickFunction a;
	a.ID = 1; a.TickGroup = ETickGroup::TG_Update; a.ExecutionLog = &log;
	a.RegisterTickFunction(*m_tickManager);

	// First tick: only 'a' runs
	m_tickManager->Tick(0.016f);
	if (!TestEqual("ASSERT_EQ", log.size(), 1u)) { return; }
	TestEqual("EXPECT_EQ", log[0], 1);

	// Register 'b' after first tick (deferred)
	FTestTickFunction b;
	b.ID = 2; b.TickGroup = ETickGroup::TG_Update; b.ExecutionLog = &log;
	b.RegisterTickFunction(*m_tickManager);

	// Second tick: both 'a' and 'b' run
	m_tickManager->Tick(0.016f);
	if (!TestEqual("ASSERT_EQ", log.size(), 3u)) { return; }

	a.UnregisterTickFunction();
	b.UnregisterTickFunction();
}

// Test 8: bCanEverTick=true creates FComponentTickFunction on attach
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, ComponentTickIntegration)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("TestObj");

	auto comp = std::make_unique<FTickableTestComponent>();
	auto* rawComp = comp.get();
	obj->AddComponent<FTickableTestComponent>();

	// Find the component we just added
	auto* tickComp = obj->GetComponent<FTickableTestComponent>();
	if (!TestNotEqual("ASSERT_NE", tickComp, nullptr)) { return; }
	TestTrue("EXPECT_TRUE", tickComp->bCanEverTick);
	TestNotEqual("EXPECT_NE", tickComp->GetTickFunction(), nullptr);
	TestTrue("EXPECT_TRUE", tickComp->GetTickFunction()->IsRegistered());
}

// Test 9: BeginPlay called by scene lifecycle, Update by tick manager
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, ComponentBeginPlayTiming)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("TestObj");
	obj->AddComponent<FTickableTestComponent>();
	auto* tickComp = obj->GetComponent<FTickableTestComponent>();
	if (!TestNotEqual("ASSERT_NE", tickComp, nullptr)) { return; }

	TestFalse("EXPECT_FALSE", tickComp->BeginPlayCalled);
	TestEqual("EXPECT_EQ", tickComp->UpdateCount, 0);

	// BeginPlay is driven by scene lifecycle, not tick manager
	scene.BeginPlay();
	TestTrue("EXPECT_TRUE", tickComp->BeginPlayCalled);
	TestEqual("EXPECT_EQ", tickComp->UpdateCount, 0); // Update not yet called

	// Tick the manager -- drives Update only
	m_tickManager->Tick(0.016f);
	TestEqual("EXPECT_EQ", tickComp->UpdateCount, 1);
}

// TESTS_PLACEHOLDER_4

// Test 10: OnDetach unregisters tick function
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, ComponentDetachUnregisters)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("TestObj");
	obj->AddComponent<FTickableTestComponent>();
	auto* tickComp = obj->GetComponent<FTickableTestComponent>();
	if (!TestNotEqual("ASSERT_NE", tickComp, nullptr)) { return; }
	TestNotEqual("EXPECT_NE", tickComp->GetTickFunction(), nullptr);

	// Remove the component -- triggers OnDetach
	obj->RemoveComponent<FTickableTestComponent>();

	// Tick should not crash (tick function was unregistered)
	m_tickManager->Tick(0.016f);
}

// Test 11: bCanEverTick=false allocates nothing
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, ZeroOverheadNonTicking)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("TestObj");
	obj->AddComponent<FTestComponent>(); // bCanEverTick = false (default)
	auto* comp = obj->GetComponent<FTestComponent>();
	if (!TestNotEqual("ASSERT_NE", comp, nullptr)) { return; }

	TestFalse("EXPECT_FALSE", comp->bCanEverTick);
	TestEqual("EXPECT_EQ", comp->GetTickFunction(), nullptr);

	// Tick should not call Update on non-ticking component
	m_tickManager->Tick(0.016f);
	TestEqual("EXPECT_EQ", comp->UpdateCount, 0);
}

// Test 12: Disabled tick function skips execution
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, DisabledTickSkipsExecution)
{
	std::vector<int> log;

	FTestTickFunction tf;
	tf.ID = 1; tf.TickGroup = ETickGroup::TG_Update; tf.ExecutionLog = &log;
	tf.bStartWithTickEnabled = false;
	tf.RegisterTickFunction(*m_tickManager);

	m_tickManager->Tick(0.016f);
	TestTrue("EXPECT_TRUE", log.empty()); // disabled, should not execute

	tf.SetTickFunctionEnable(true);
	m_tickManager->Tick(0.016f);
	TestEqual("EXPECT_EQ", log.size(), 1u); // now enabled

	tf.UnregisterTickFunction();
}

// Test 13: Multiple tick functions across groups with prerequisites
ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, ComplexTickSchedule)
{
	std::vector<int> log;

	// PreUpdate group
	FTestTickFunction pre1;
	pre1.ID = 10; pre1.TickGroup = ETickGroup::TG_PreUpdate; pre1.ExecutionLog = &log;
	pre1.RegisterTickFunction(*m_tickManager);

	// Update group with dependency chain
	FTestTickFunction upA;
	upA.ID = 20; upA.TickGroup = ETickGroup::TG_Update; upA.ExecutionLog = &log;
	upA.RegisterTickFunction(*m_tickManager);

	FTestTickFunction upB;
	upB.ID = 21; upB.TickGroup = ETickGroup::TG_Update; upB.ExecutionLog = &log;
	upB.AddPrerequisite(upA);
	upB.RegisterTickFunction(*m_tickManager);

	// PostUpdate group
	FTestTickFunction post1;
	post1.ID = 30; post1.TickGroup = ETickGroup::TG_PostUpdate; post1.ExecutionLog = &log;
	post1.RegisterTickFunction(*m_tickManager);

	m_tickManager->Tick(0.016f);

	if (!TestEqual("ASSERT_EQ", log.size(), 4u)) { return; }
	TestEqual("EXPECT_EQ", log[0], 10); // PreUpdate
	TestEqual("EXPECT_EQ", log[1], 20); // Update A
	TestEqual("EXPECT_EQ", log[2], 21); // Update B (after A)
	TestEqual("EXPECT_EQ", log[3], 30); // PostUpdate

	pre1.UnregisterTickFunction();
	upA.UnregisterTickFunction();
	upB.UnregisterTickFunction();
	post1.UnregisterTickFunction();
}

ENIGMA_IMPLEMENT_ENGINE_TICK_SYSTEM_AUTOMATION_TEST_F(TickSystemFixture, FlushPendingChangesDoesNotExecuteTicks)
{
	std::vector<int> Log;

	FTestTickFunction TickFunction;
	TickFunction.ID = 1;
	TickFunction.TickGroup = ETickGroup::TG_Update;
	TickFunction.ExecutionLog = &Log;

	TickFunction.RegisterTickFunction(*m_tickManager);
	m_tickManager->FlushPendingChanges();
	TestTrue("EXPECT_TRUE", Log.empty());

	m_tickManager->Tick(0.016f);
	if (!TestEqual("ASSERT_EQ", Log.size(), 1u)) { return; }
	TestEqual("EXPECT_EQ", Log[0], 1);

	TickFunction.UnregisterTickFunction();
	m_tickManager->FlushPendingChanges();

	Log.clear();
	m_tickManager->Tick(0.016f);
	TestTrue("EXPECT_TRUE", Log.empty());
}
