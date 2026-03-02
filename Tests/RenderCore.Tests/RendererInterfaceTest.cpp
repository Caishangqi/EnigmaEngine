// Copyright EnigmaEngine. All Rights Reserved.

/// @file RendererInterfaceTest.cpp
/// @brief Unit tests for renderer module retrieval and interface contracts.

#include <gtest/gtest.h>

#include "EngineModule.h"
#include "RenderCore/RendererInterface.h"
#include "RenderCore/AsciiRendererInterface.h"
#include "RenderCore/AsciiBlendState.h"
#include "RenderCore/AsciiCell.h"
#include "RenderCore/AsciiSprite.h"
#include "SceneView/SceneView.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleMacros.h"

namespace Enigma
{

// ---------------------------------------------------------------
// Mock renderer for registration tests
// ---------------------------------------------------------------
class FMockRendererModule : public IAsciiRendererModule
{
public:
	void StartupModule() override {}
	void ShutdownModule() override {}

	void Initialize(FGenericWindow*) override {}
	void BeginFrame() override { ++BeginFrameCount; }
	void EndFrame() override { ++EndFrameCount; }
	void SetActiveView(const FSceneView&) override {}
	int32_t GetFrameBufferWidth() const override { return 80; }
	int32_t GetFrameBufferHeight() const override { return 25; }

	void DrawCell(int32_t, int32_t, int32_t, FAsciiCell) override {}
	void DrawSprite(int32_t, int32_t, int32_t,
	                const FAsciiSprite&) override {}
	void DrawText(int32_t, int32_t, int32_t,
	              const char*, FColor, FColor) override {}
	void FillRect(int32_t, int32_t, int32_t, int32_t,
	              int32_t, FAsciiCell) override {}
	void DrawBox(int32_t, int32_t, int32_t, int32_t,
	             int32_t, FColor, FColor) override {}
	void SetBlendState(const FAsciiBlendState&) override {}

	int BeginFrameCount = 0;
	int EndFrameCount = 0;
};

// ---------------------------------------------------------------
// Static registration of mock renderer as "Renderer" module
// ---------------------------------------------------------------
static IModuleInterface* CreateMockRendererModule()
{
	return new FMockRendererModule();
}

static FModuleInitializerEntry s_mockRendererEntry("Renderer", CreateMockRendererModule);

} // namespace Enigma

// ---------------------------------------------------------------
// Tests
// ---------------------------------------------------------------

using namespace Enigma;

/// Before LoadModule, TryGetRendererModule returns nullptr.
TEST(RendererInterfaceTest, TryGetRendererModule_ReturnsNullptr_WhenNotLoaded)
{
	// At this point no module has been loaded via LoadModule yet.
	// TryGetRendererModule only checks GetModule (no LoadModule call).
	// Since the mock is registered but not loaded, it should return nullptr.
	IRendererModule* renderer = TryGetRendererModule();
	EXPECT_EQ(renderer, nullptr);
}

/// After LoadModule, GetRendererModule returns a valid reference.
TEST(RendererInterfaceTest, GetRendererModule_ReturnsValidRef_AfterLoad)
{
	IRendererModule& renderer = GetRendererModule();
	// Should not crash and should return a valid object.
	EXPECT_EQ(renderer.GetFrameBufferWidth(), 80);
	EXPECT_EQ(renderer.GetFrameBufferHeight(), 25);
}

/// After LoadModule, TryGetRendererModule returns non-null.
TEST(RendererInterfaceTest, TryGetRendererModule_ReturnsNonNull_AfterLoad)
{
	// Ensure the module is loaded first (each CTest case runs in its own process).
	GetRendererModule();

	IRendererModule* renderer = TryGetRendererModule();
	ASSERT_NE(renderer, nullptr);
	EXPECT_EQ(renderer->GetFrameBufferWidth(), 80);
}

/// GetModuleChecked<IAsciiRendererModule> returns the correct sub-interface.
TEST(RendererInterfaceTest, GetModuleChecked_ReturnsAsciiInterface)
{
	// Ensure the module is loaded first (each CTest case runs in its own process).
	GetRendererModule();

	auto& asciiRenderer = FModuleManager::Get().GetModuleChecked<IAsciiRendererModule>("Renderer");

	// Verify it's the same object as GetRendererModule.
	IRendererModule& genericRenderer = GetRendererModule();
	EXPECT_EQ(&asciiRenderer, &genericRenderer);

	// Verify frame buffer dimensions through the sub-interface.
	EXPECT_EQ(asciiRenderer.GetFrameBufferWidth(), 80);
	EXPECT_EQ(asciiRenderer.GetFrameBufferHeight(), 25);
}

/// BeginFrame/EndFrame lifecycle works through the interface.
TEST(RendererInterfaceTest, BeginEndFrame_Lifecycle)
{
	// Ensure the module is loaded first (each CTest case runs in its own process).
	GetRendererModule();

	auto& asciiRenderer = FModuleManager::Get().GetModuleChecked<IAsciiRendererModule>("Renderer");
	auto* mock = dynamic_cast<FMockRendererModule*>(&asciiRenderer);
	ASSERT_NE(mock, nullptr);

	int beforeBegin = mock->BeginFrameCount;
	int beforeEnd = mock->EndFrameCount;

	asciiRenderer.BeginFrame();
	EXPECT_EQ(mock->BeginFrameCount, beforeBegin + 1);

	asciiRenderer.EndFrame();
	EXPECT_EQ(mock->EndFrameCount, beforeEnd + 1);
}
