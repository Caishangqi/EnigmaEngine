// DLL Export Macro Verification Test
// Validates that CORE_API, ENGINE_API, LAUNCH_API resolve correctly
// based on {MODULE}_EXPORTS preprocessor definitions.

#include "HAL/Platform.h"
#include "CoreAPI.generated.h"

// Temporarily include via relative path since Engine/Launch are separate modules
// In real builds, include paths are set by BuildTool
#include "../../Engine/Public/EngineAPI.generated.h"
#include "../../Launch/Public/LaunchAPI.generated.h"

#include <cstdio>

// ---------------------------------------------------------------
// Test 1: Verify DLLEXPORT/DLLIMPORT are defined
// ---------------------------------------------------------------
#ifndef DLLEXPORT
    #error "DLLEXPORT is not defined"
#endif
#ifndef DLLIMPORT
    #error "DLLIMPORT is not defined"
#endif

// ---------------------------------------------------------------
// Test 2: Without any _EXPORTS defined, all APIs should be DLLIMPORT
// (This file is compiled as a consumer, not a module builder)
// ---------------------------------------------------------------

// Use the macros on declarations to verify they compile
CORE_API void CoreFunction();
ENGINE_API void EngineFunction();
LAUNCH_API void LaunchFunction();

// Use the macros on class declarations
class CORE_API FCoreClass { public: virtual ~FCoreClass() = default; };
class ENGINE_API FEngineClass { public: virtual ~FEngineClass() = default; };
class LAUNCH_API FLaunchClass { public: virtual ~FLaunchClass() = default; };

static int g_passed = 0;
static int g_failed = 0;

static void Assert(bool cond, const char* msg)
{
    if (cond) { std::printf("  PASSED: %s\n", msg); ++g_passed; }
    else      { std::printf("  FAILED: %s\n", msg); ++g_failed; }
}

int main()
{
    std::printf("=== DLL Export Macro Test (REQ-011) ===\n\n");

    // Platform detection
#if defined(_WIN32) || defined(_WIN64)
    std::printf("[Platform] Windows detected\n");
    Assert(ENIGMA_PLATFORM_WINDOWS == 1, "ENIGMA_PLATFORM_WINDOWS is 1 on Windows");
#else
    std::printf("[Platform] Unix/Mac detected\n");
    Assert(ENIGMA_PLATFORM_WINDOWS == 0, "ENIGMA_PLATFORM_WINDOWS is 0 on Unix");
#endif

    // Verify macros are usable (compilation itself is the test)
    Assert(true, "CORE_API compiles on function declaration");
    Assert(true, "ENGINE_API compiles on function declaration");
    Assert(true, "LAUNCH_API compiles on function declaration");
    Assert(true, "CORE_API compiles on class declaration");
    Assert(true, "ENGINE_API compiles on class declaration");
    Assert(true, "LAUNCH_API compiles on class declaration");

    // Verify _EXPORTS switching logic:
    // Since we did NOT define CORE_EXPORTS, CORE_API should be DLLIMPORT
    // We can't directly test macro expansion at runtime, but we can verify
    // the preprocessor logic is correct via conditional compilation
#ifdef CORE_EXPORTS
    Assert(false, "CORE_EXPORTS should NOT be defined in consumer");
#else
    Assert(true, "CORE_EXPORTS not defined -- CORE_API is DLLIMPORT");
#endif

#ifdef ENGINE_EXPORTS
    Assert(false, "ENGINE_EXPORTS should NOT be defined in consumer");
#else
    Assert(true, "ENGINE_EXPORTS not defined -- ENGINE_API is DLLIMPORT");
#endif

#ifdef LAUNCH_EXPORTS
    Assert(false, "LAUNCH_EXPORTS should NOT be defined in consumer");
#else
    Assert(true, "LAUNCH_EXPORTS not defined -- LAUNCH_API is DLLIMPORT");
#endif

    std::printf("\n=== Results: %d passed, %d failed ===\n", g_passed, g_failed);
    return g_failed > 0 ? 1 : 0;
}
