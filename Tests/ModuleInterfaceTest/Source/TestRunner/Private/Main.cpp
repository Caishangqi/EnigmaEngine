// ModuleInterfaceTest runner.
// Dynamically loads TestModule.dll and TestGameModule.dll,
// then uses FModuleInitializerEntry::FindModule to locate and
// instantiate modules via the self-registering linked list.

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleInitializerEntry.h"

#include <cstdio>
#include <cstring>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    using LibHandle = HMODULE;
    #define LoadLib(path) LoadLibraryA(path)
    #define FreeLib(lib) FreeLibrary(lib)
#else
    #include <dlfcn.h>
    using LibHandle = void*;
    #define LoadLib(path) dlopen(path, RTLD_NOW)
    #define FreeLib(lib) dlclose(lib)
#endif

static int g_testsPassed = 0;
static int g_testsFailed = 0;

static void Assert(bool condition, const char* message)
{
    if (condition)
    {
        std::printf("  PASSED: %s\n", message);
        ++g_testsPassed;
    }
    else
    {
        std::printf("  FAILED: %s\n", message);
        ++g_testsFailed;
    }
}

static LibHandle LoadModule(const char* name)
{
#ifdef _WIN32
    char path[512];
    std::snprintf(path, sizeof(path), "%s.dll", name);
#elif defined(__APPLE__)
    char path[512];
    std::snprintf(path, sizeof(path), "lib%s.dylib", name);
#else
    char path[512];
    std::snprintf(path, sizeof(path), "lib%s.so", name);
#endif
    auto lib = LoadLib(path);
    if (!lib)
    {
        std::printf("  ERROR: Failed to load %s\n", path);
    }
    return lib;
}

int main()
{
    std::printf("=== ModuleInterface Test (Self-Registering Pattern) ===\n\n");

    // Before loading any DLL, FindModule should return nullptr
    Assert(Enigma::FModuleInitializerEntry::FindModule("TestModule") == nullptr,
        "FindModule('TestModule') is null before DLL load");

    // -- Test 1: TestModule (IMPLEMENT_MODULE) --
    std::printf("\n[Test 1] TestModule (IMPLEMENT_MODULE)\n");
    {
        auto lib = LoadModule("TestModule");
        Assert(lib != nullptr, "TestModule DLL loaded");

        if (lib)
        {
            // After loading, the static FModuleInitializerEntry should have
            // self-registered into the global linked list
            auto initFn = Enigma::FModuleInitializerEntry::FindModule("TestModule");
            Assert(initFn != nullptr, "FindModule('TestModule') found after DLL load");

            if (initFn)
            {
                auto* mod = initFn();
                Assert(mod != nullptr, "Initializer returned non-null module");

                if (mod)
                {
                    mod->StartupModule();
                    mod->ShutdownModule();

                    Assert(mod->SupportsDynamicReloading() == false,
                        "SupportsDynamicReloading returns false");
                    Assert(mod->IsGameModule() == false,
                        "IsGameModule returns false (engine module)");

                    delete mod;
                }
            }

            FreeLib(lib);

            // After unloading, the destructor should have removed the entry
            auto afterUnload = Enigma::FModuleInitializerEntry::FindModule("TestModule");
            Assert(afterUnload == nullptr,
                "FindModule('TestModule') is null after DLL unload");
        }
    }

    // -- Test 2: TestGameModule (IMPLEMENT_GAME_MODULE) --
    std::printf("\n[Test 2] TestGameModule (IMPLEMENT_GAME_MODULE)\n");
    {
        auto lib = LoadModule("TestGameModule");
        Assert(lib != nullptr, "TestGameModule DLL loaded");

        if (lib)
        {
            auto initFn = Enigma::FModuleInitializerEntry::FindModule("TestGameModule");
            Assert(initFn != nullptr, "FindModule('TestGameModule') found after DLL load");

            if (initFn)
            {
                auto* mod = initFn();
                Assert(mod != nullptr, "Initializer returned non-null module");

                if (mod)
                {
                    mod->StartupModule();
                    mod->ShutdownModule();

                    Assert(mod->IsGameModule() == true,
                        "IsGameModule returns true (game module via virtual)");

                    delete mod;
                }
            }

            FreeLib(lib);

            auto afterUnload = Enigma::FModuleInitializerEntry::FindModule("TestGameModule");
            Assert(afterUnload == nullptr,
                "FindModule('TestGameModule') is null after DLL unload");
        }
    }

    // -- Test 3: Load both simultaneously --
    std::printf("\n[Test 3] Both modules loaded simultaneously\n");
    {
        auto lib1 = LoadModule("TestModule");
        auto lib2 = LoadModule("TestGameModule");

        Assert(lib1 != nullptr && lib2 != nullptr, "Both DLLs loaded");

        if (lib1 && lib2)
        {
            auto fn1 = Enigma::FModuleInitializerEntry::FindModule("TestModule");
            auto fn2 = Enigma::FModuleInitializerEntry::FindModule("TestGameModule");

            Assert(fn1 != nullptr, "FindModule('TestModule') found");
            Assert(fn2 != nullptr, "FindModule('TestGameModule') found");
            Assert(fn1 != fn2, "Different initializer functions");

            // Unknown module should return nullptr
            auto fnUnknown = Enigma::FModuleInitializerEntry::FindModule("NonExistent");
            Assert(fnUnknown == nullptr, "FindModule('NonExistent') returns null");
        }

        if (lib2) FreeLib(lib2);
        if (lib1) FreeLib(lib1);
    }

    std::printf("\n=== Results: %d passed, %d failed ===\n",
        g_testsPassed, g_testsFailed);

    return g_testsFailed > 0 ? 1 : 0;
}
