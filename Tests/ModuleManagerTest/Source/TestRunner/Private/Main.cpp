// ModuleManager Test Runner.
// Validates FModuleManager singleton: LoadModule, UnloadModule,
// IsModuleLoaded, GetModule, GetModuleChecked, UnloadAllModules.

#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

static int g_passed = 0;
static int g_failed = 0;

static void Assert(bool cond, const char* msg)
{
    if (cond) { std::printf("  PASSED: %s\n", msg); ++g_passed; }
    else      { std::printf("  FAILED: %s\n", msg); ++g_failed; }
}

static std::filesystem::path GetExecutableDirectory()
{
#ifdef _WIN32
    char buffer[MAX_PATH] = {};
    DWORD length = ::GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length > 0)
    {
        return std::filesystem::path(buffer).parent_path();
    }
#endif

    return std::filesystem::current_path();
}

int main()
{
    std::printf("=== FModuleManager Test ===\n");

    auto& mgr = Enigma::FModuleManager::Get();

    // -- Test 1: Singleton identity --
    std::printf("\n[Test 1] Singleton identity\n");
    {
        auto& mgr2 = Enigma::FModuleManager::Get();
        Assert(&mgr == &mgr2, "Get() returns same instance");
    }

    // -- Test 2: IsModuleLoaded before any load --
    std::printf("\n[Test 2] Pre-load state\n");
    {
        Assert(!mgr.IsModuleLoaded("TestModule"),
            "TestModule not loaded initially");
        Assert(!mgr.IsModuleLoaded("TestGameModule"),
            "TestGameModule not loaded initially");
        Assert(mgr.GetModule("TestModule") == nullptr,
            "GetModule returns nullptr for unloaded module");
    }

    // -- Test 3: LoadModule --
    std::printf("\n[Test 3] LoadModule\n");
    {
        auto* mod = mgr.LoadModule("TestModule");
        Assert(mod != nullptr, "LoadModule('TestModule') returns non-null");
        Assert(mgr.IsModuleLoaded("TestModule"),
            "IsModuleLoaded('TestModule') is true after load");

        // Load again -- should return same instance
        auto* mod2 = mgr.LoadModule("TestModule");
        Assert(mod == mod2, "LoadModule returns same instance on double load");
    }

    // -- Test 4: GetModule --
    std::printf("\n[Test 4] GetModule\n");
    {
        auto* mod = mgr.GetModule("TestModule");
        Assert(mod != nullptr, "GetModule('TestModule') returns non-null");
        Assert(mod->SupportsDynamicReloading() == false,
            "Module method call works (SupportsDynamicReloading=false)");
        Assert(mod->IsGameModule() == false,
            "IsGameModule returns false for engine module");
    }

    // -- Test 5: GetModuleChecked --
    std::printf("\n[Test 5] GetModuleChecked<T>\n");
    {
        auto& ref = mgr.GetModuleChecked<Enigma::IModuleInterface>("TestModule");
        Assert(&ref == mgr.GetModule("TestModule"),
            "GetModuleChecked returns reference to same object");
    }

    // -- Test 6: UnloadModule --
    std::printf("\n[Test 6] UnloadModule\n");
    {
        bool ok = mgr.UnloadModule("TestModule");
        Assert(ok, "UnloadModule('TestModule') returns true");
        Assert(!mgr.IsModuleLoaded("TestModule"),
            "IsModuleLoaded is false after unload");
        Assert(mgr.GetModule("TestModule") == nullptr,
            "GetModule returns nullptr after unload");

        // Unload again -- should return false
        bool ok2 = mgr.UnloadModule("TestModule");
        Assert(!ok2, "UnloadModule on already-unloaded returns false");
    }

    // -- Test 7: Load game module --
    std::printf("\n[Test 7] Load game module\n");
    {
        auto* mod = mgr.LoadModule("TestGameModule");
        Assert(mod != nullptr, "LoadModule('TestGameModule') returns non-null");
        Assert(mod->IsGameModule() == true,
            "IsGameModule returns true for game module");
    }

    // -- Test 8: Load both, then UnloadAllModules --
    std::printf("\n[Test 8] UnloadAllModules (reverse order)\n");
    {
        // TestGameModule already loaded from Test 7.
        // Load TestModule again.
        auto* mod1 = mgr.LoadModule("TestModule");
        Assert(mod1 != nullptr, "TestModule re-loaded");
        Assert(mgr.IsModuleLoaded("TestModule"), "TestModule is loaded");
        Assert(mgr.IsModuleLoaded("TestGameModule"), "TestGameModule is loaded");

        mgr.UnloadAllModules();

        Assert(!mgr.IsModuleLoaded("TestModule"),
            "TestModule unloaded after UnloadAllModules");
        Assert(!mgr.IsModuleLoaded("TestGameModule"),
            "TestGameModule unloaded after UnloadAllModules");
    }

    // -- Test 9: Manifest scan records DLL path --
    std::printf("\n[Test 9] Manifest scan records DLL path\n");
    {
        auto exeDir = GetExecutableDirectory();
        auto manifestPath = exeDir / "SnapshotTarget.modules";

        {
            std::ofstream manifest(manifestPath);
            manifest
                << "{\n"
                << "  \"BuildId\": \"TEST\",\n"
                << "  \"Modules\": {\n"
                << "    \"TestModule\": \"TestModule.dll\"\n"
                << "  }\n"
                << "}\n";
        }

        mgr.SetTargetName("SnapshotTarget");
        mgr.ScanDllsFromDirectory(exeDir.string());
        mgr.LoadAllRegisteredModules();

        Assert(mgr.IsModuleLoaded("TestModule"),
            "TestModule loaded from .modules manifest");

        std::string dllPath = mgr.GetModuleDllPath("TestModule");
        Assert(!dllPath.empty(),
            "GetModuleDllPath returns manifest DLL path");
        Assert(std::filesystem::path(dllPath).filename().string() == "TestModule.dll",
            "GetModuleDllPath keeps the manifest-selected DLL filename");

        mgr.UnloadAllModules();
        std::error_code ec;
        std::filesystem::remove(manifestPath, ec);
    }

    // -- Test 10: Load non-existent module --
    std::printf("\n[Test 10] Load non-existent module\n");
    {
        auto* mod = mgr.LoadModule("NonExistentModule");
        Assert(mod == nullptr,
            "LoadModule('NonExistentModule') returns nullptr");
    }

    std::printf("\n=== Results: %d passed, %d failed ===\n",
        g_passed, g_failed);

    return g_failed > 0 ? 1 : 0;
}
