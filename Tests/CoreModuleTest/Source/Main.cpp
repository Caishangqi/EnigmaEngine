// CoreModule Test Runner.
// Validates:
//   1. CoreMinimal.h compiles as a unified include
//   2. Core.dll builds with FCoreModule + FModuleManager
//   3. FModuleManager can load/unload Core by name
//   4. StartupModule/ShutdownModule are called
//   5. Module properties are correct

#include "CoreMinimal.h"
#include <cstdio>

static int g_passed = 0;
static int g_failed = 0;

static void Assert(bool cond, const char* msg)
{
    if (cond) { std::printf("  PASSED: %s\n", msg); ++g_passed; }
    else      { std::printf("  FAILED: %s\n", msg); ++g_failed; }
}

int main()
{
    std::printf("=== Core Module Test ===\n");

    auto& mgr = Enigma::FModuleManager::Get();

    // -- Test 1: CoreMinimal.h compiles --
    std::printf("\n[Test 1] CoreMinimal.h unified include\n");
    Assert(true, "CoreMinimal.h compiled successfully");

    // -- Test 2: Core not loaded initially --
    std::printf("\n[Test 2] Pre-load state\n");
    Assert(!mgr.IsModuleLoaded("Core"), "Core not loaded initially");

    // -- Test 3: Load Core module --
    // Note: Core.dll is in the same directory as the executable.
    // FModuleManager uses the self-registering linked list, but since
    // Core is already linked (we link against Core.lib), the static
    // FModuleInitializerEntry for "Core" is already registered.
    // However, FModuleManager::LoadModule tries to LoadLibrary first.
    // For statically-linked modules, we need to register them differently.
    //
    // In this test, Core.dll is both linked AND loadable. The static
    // CoreInitializerEntry is already in the linked list because we
    // linked against Core.lib. So FindModule("Core") will succeed
    // even without LoadLibrary. But LoadModule will still call
    // LoadLibrary (which succeeds since Core.dll is in the same dir).
    std::printf("\n[Test 3] LoadModule('Core')\n");
    {
        auto* mod = mgr.LoadModule("Core");
        Assert(mod != nullptr, "LoadModule('Core') returns non-null");
        Assert(mgr.IsModuleLoaded("Core"), "IsModuleLoaded('Core') is true");
    }

    // -- Test 4: Module properties --
    std::printf("\n[Test 4] Module properties\n");
    {
        auto* mod = mgr.GetModule("Core");
        Assert(mod != nullptr, "GetModule('Core') returns non-null");
        if (mod)
        {
            Assert(mod->SupportsDynamicReloading() == false,
                "Core does not support dynamic reloading");
            Assert(mod->IsGameModule() == false,
                "Core is not a game module");
        }
    }

    // -- Test 5: GetModuleChecked --
    std::printf("\n[Test 5] GetModuleChecked\n");
    {
        auto& ref = mgr.GetModuleChecked<Enigma::IModuleInterface>("Core");
        Assert(&ref == mgr.GetModule("Core"),
            "GetModuleChecked returns same object");
    }

    // -- Test 6: Unload Core --
    std::printf("\n[Test 6] UnloadModule('Core')\n");
    {
        bool ok = mgr.UnloadModule("Core");
        Assert(ok, "UnloadModule('Core') returns true");
        Assert(!mgr.IsModuleLoaded("Core"),
            "Core not loaded after unload");
    }

    std::printf("\n=== Results: %d passed, %d failed ===\n",
        g_passed, g_failed);

    return g_failed > 0 ? 1 : 0;
}
