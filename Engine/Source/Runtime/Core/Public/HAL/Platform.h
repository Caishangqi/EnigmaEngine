// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

// -------------------------------------------------------------
// Platform abstraction for DLL export/import
// -------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
    #define ENIGMA_PLATFORM_WINDOWS 1
    #define DLLEXPORT __declspec(dllexport)
    #define DLLIMPORT __declspec(dllimport)
#elif defined(__GNUC__) || defined(__clang__)
    #define ENIGMA_PLATFORM_WINDOWS 0
    #define DLLEXPORT __attribute__((visibility("default")))
    #define DLLIMPORT
#else
    #define ENIGMA_PLATFORM_WINDOWS 0
    #define DLLEXPORT
    #define DLLIMPORT
#endif

// -------------------------------------------------------------
// Build configuration macros (REQ-002)
//
// Exactly one of these is defined to 1 by BuildTool (via CMake
// add_compile_definitions). All others default to 0.
//
//   ENIGMA_BUILD_DEBUG       -- Full debug, no optimization
//   ENIGMA_BUILD_DEBUGGAME   -- Engine optimized, game debug
//   ENIGMA_BUILD_DEVELOPMENT -- Moderate optimization, debug features
//   ENIGMA_BUILD_SHIPPING    -- Full optimization, no debug code
//   ENIGMA_BUILD_TEST        -- Full optimization, debug symbols retained
// -------------------------------------------------------------

#if !defined(ENIGMA_BUILD_DEBUG)
    #define ENIGMA_BUILD_DEBUG 0
#endif

#if !defined(ENIGMA_BUILD_DEBUGGAME)
    #define ENIGMA_BUILD_DEBUGGAME 0
#endif

#if !defined(ENIGMA_BUILD_DEVELOPMENT)
    #define ENIGMA_BUILD_DEVELOPMENT 0
#endif

#if !defined(ENIGMA_BUILD_SHIPPING)
    #define ENIGMA_BUILD_SHIPPING 0
#endif

#if !defined(ENIGMA_BUILD_TEST)
    #define ENIGMA_BUILD_TEST 0
#endif
