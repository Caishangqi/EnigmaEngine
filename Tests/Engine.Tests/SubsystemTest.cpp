// Copyright EnigmaEngine. All Rights Reserved.

/// @file SubsystemTest.cpp
/// @brief Unit tests for ISubsystem and FSubsystemCollection.

#include <gtest/gtest.h>
#include "Subsystems/SubsystemCollection.h"

#include <string>
#include <vector>

using namespace Enigma;

// =================================================================
// Mock subsystems for testing
// =================================================================

/// Shared log for verifying lifecycle call order.
static std::vector<std::string> g_lifecycleLog;

class FMockSubsystemA : public ISubsystem
{
public:
	static const char* GetStaticName() { return "FMockSubsystemA"; }
	const char* GetName() const override { return GetStaticName(); }
	bool IsTickable() const override { return true; }
	int32_t GetTickPriority() const override { return 100; }

	void Initialize(FSubsystemCollection& collection) override
	{
		g_lifecycleLog.push_back("A::Initialize");
	}
	void PostInitialize() override
	{
		g_lifecycleLog.push_back("A::PostInitialize");
	}
	void Tick(float dt) override
	{
		g_lifecycleLog.push_back("A::Tick");
	}
	void Deinitialize() override
	{
		g_lifecycleLog.push_back("A::Deinitialize");
	}
};

class FMockSubsystemB : public ISubsystem
{
public:
	static const char* GetStaticName() { return "FMockSubsystemB"; }
	const char* GetName() const override { return GetStaticName(); }
	bool IsTickable() const override { return true; }
	int32_t GetTickPriority() const override { return 200; } // higher than A

	void Initialize(FSubsystemCollection& collection) override
	{
		g_lifecycleLog.push_back("B::Initialize");
	}
	void PostInitialize() override
	{
		g_lifecycleLog.push_back("B::PostInitialize");
	}
	void Tick(float dt) override
	{
		g_lifecycleLog.push_back("B::Tick");
	}
	void Deinitialize() override
	{
		g_lifecycleLog.push_back("B::Deinitialize");
	}
};

/// Subsystem that returns false from ShouldCreateSubsystem.
class FMockSubsystemSkipped : public ISubsystem
{
public:
	static const char* GetStaticName() { return "FMockSubsystemSkipped"; }
	const char* GetName() const override { return GetStaticName(); }
	bool ShouldCreateSubsystem() const override { return false; }
};

/// Subsystem that depends on FMockSubsystemA via InitializeDependency.
class FMockSubsystemDependent : public ISubsystem
{
public:
	static const char* GetStaticName() { return "FMockSubsystemDependent"; }
	const char* GetName() const override { return GetStaticName(); }

	void Initialize(FSubsystemCollection& collection) override
	{
		// Request dependency — A should be initialized before us
		collection.InitializeDependency<FMockSubsystemA>();
		g_lifecycleLog.push_back("Dependent::Initialize");
	}
	void Deinitialize() override
	{
		g_lifecycleLog.push_back("Dependent::Deinitialize");
	}
};

/// Non-tickable subsystem.
class FMockSubsystemNonTickable : public ISubsystem
{
public:
	static const char* GetStaticName() { return "FMockSubsystemNonTickable"; }
	const char* GetName() const override { return GetStaticName(); }
	bool IsTickable() const override { return false; }

	void Initialize(FSubsystemCollection& collection) override
	{
		g_lifecycleLog.push_back("NonTickable::Initialize");
	}
	void Tick(float) override
	{
		g_lifecycleLog.push_back("NonTickable::Tick"); // should never be called
	}
};

// Helper to clear log before each test
class SubsystemCollectionTest : public ::testing::Test
{
protected:
	void SetUp() override { g_lifecycleLog.clear(); }
};

// =================================================================
// Tests
// =================================================================

TEST_F(SubsystemCollectionTest, FullLifecycleOrder)
{
	FSubsystemCollection collection;
	collection.RegisterSubsystem<FMockSubsystemA>();
	collection.Initialize();
	collection.Tick(0.016f);
	collection.Deinitialize();

	ASSERT_EQ(g_lifecycleLog.size(), 4u);
	EXPECT_EQ(g_lifecycleLog[0], "A::Initialize");
	EXPECT_EQ(g_lifecycleLog[1], "A::PostInitialize");
	EXPECT_EQ(g_lifecycleLog[2], "A::Tick");
	EXPECT_EQ(g_lifecycleLog[3], "A::Deinitialize");
}

TEST_F(SubsystemCollectionTest, DeinitializeInReverseOrder)
{
	FSubsystemCollection collection;
	collection.RegisterSubsystem<FMockSubsystemA>();
	collection.RegisterSubsystem<FMockSubsystemB>();
	collection.Initialize();
	collection.Deinitialize();

	// A registered first → initialized first → deinitialized last
	// Find deinit entries
	std::vector<std::string> deinits;
	for (const auto& entry : g_lifecycleLog)
	{
		if (entry.find("Deinitialize") != std::string::npos)
		{
			deinits.push_back(entry);
		}
	}
	ASSERT_EQ(deinits.size(), 2u);
	EXPECT_EQ(deinits[0], "B::Deinitialize"); // reverse of init order
	EXPECT_EQ(deinits[1], "A::Deinitialize");
}

TEST_F(SubsystemCollectionTest, GetSubsystemReturnsCorrectPointer)
{
	FSubsystemCollection collection;
	collection.RegisterSubsystem<FMockSubsystemA>();
	collection.RegisterSubsystem<FMockSubsystemB>();
	collection.Initialize();

	auto* a = collection.GetSubsystem<FMockSubsystemA>();
	auto* b = collection.GetSubsystem<FMockSubsystemB>();
	EXPECT_NE(a, nullptr);
	EXPECT_NE(b, nullptr);
	EXPECT_STREQ(a->GetName(), "FMockSubsystemA");
	EXPECT_STREQ(b->GetName(), "FMockSubsystemB");

	collection.Deinitialize();
}

TEST_F(SubsystemCollectionTest, GetSubsystemUnregisteredReturnsNull)
{
	FSubsystemCollection collection;
	collection.RegisterSubsystem<FMockSubsystemA>();
	collection.Initialize();

	auto* b = collection.GetSubsystem<FMockSubsystemB>();
	EXPECT_EQ(b, nullptr);

	collection.Deinitialize();
}

TEST_F(SubsystemCollectionTest, TickPriorityOrder)
{
	FSubsystemCollection collection;
	// Register A (priority 100) before B (priority 200)
	collection.RegisterSubsystem<FMockSubsystemA>();
	collection.RegisterSubsystem<FMockSubsystemB>();
	collection.Initialize();
	g_lifecycleLog.clear(); // clear init logs
	collection.Tick(0.016f);

	// B has higher priority (200) → ticks first
	ASSERT_EQ(g_lifecycleLog.size(), 2u);
	EXPECT_EQ(g_lifecycleLog[0], "B::Tick");
	EXPECT_EQ(g_lifecycleLog[1], "A::Tick");

	collection.Deinitialize();
}

TEST_F(SubsystemCollectionTest, ShouldCreateSubsystemFalseSkipsCreation)
{
	FSubsystemCollection collection;
	collection.RegisterSubsystem<FMockSubsystemSkipped>();
	collection.Initialize();

	auto* s = collection.GetSubsystem<FMockSubsystemSkipped>();
	EXPECT_EQ(s, nullptr);

	collection.Deinitialize();
}

TEST_F(SubsystemCollectionTest, DuplicateRegistrationIgnored)
{
	FSubsystemCollection collection;
	collection.RegisterSubsystem<FMockSubsystemA>();
	collection.RegisterSubsystem<FMockSubsystemA>(); // duplicate
	collection.Initialize();

	// Should only have one Initialize call
	int initCount = 0;
	for (const auto& entry : g_lifecycleLog)
	{
		if (entry == "A::Initialize") ++initCount;
	}
	EXPECT_EQ(initCount, 1);

	collection.Deinitialize();
}

TEST_F(SubsystemCollectionTest, InitializeDependencyEnsuresOrder)
{
	FSubsystemCollection collection;
	// Register Dependent BEFORE A — dependency resolution should still init A first
	collection.RegisterSubsystem<FMockSubsystemDependent>();
	collection.RegisterSubsystem<FMockSubsystemA>();
	collection.Initialize();

	// A::Initialize must appear before Dependent::Initialize
	int aIdx = -1, depIdx = -1;
	for (int i = 0; i < static_cast<int>(g_lifecycleLog.size()); ++i)
	{
		if (g_lifecycleLog[i] == "A::Initialize") aIdx = i;
		if (g_lifecycleLog[i] == "Dependent::Initialize") depIdx = i;
	}
	EXPECT_NE(aIdx, -1);
	EXPECT_NE(depIdx, -1);
	EXPECT_LT(aIdx, depIdx);

	collection.Deinitialize();
}

TEST_F(SubsystemCollectionTest, NonTickableSubsystemNotTicked)
{
	FSubsystemCollection collection;
	collection.RegisterSubsystem<FMockSubsystemNonTickable>();
	collection.Initialize();
	g_lifecycleLog.clear();
	collection.Tick(0.016f);

	// NonTickable::Tick should NOT appear
	for (const auto& entry : g_lifecycleLog)
	{
		EXPECT_NE(entry, "NonTickable::Tick");
	}

	collection.Deinitialize();
}

TEST_F(SubsystemCollectionTest, ForEachSubsystemVisitsAll)
{
	FSubsystemCollection collection;
	collection.RegisterSubsystem<FMockSubsystemA>();
	collection.RegisterSubsystem<FMockSubsystemB>();
	collection.Initialize();

	int count = 0;
	collection.ForEachSubsystem([&](ISubsystem*) { ++count; });
	EXPECT_EQ(count, 2);

	collection.Deinitialize();
}
