# EnhancedInput.Tests: GoogleTest-based unit tests for the EnhancedInput plugin.
# Standalone CMake project, independent from BuildTool module system.
cmake_minimum_required(VERSION 3.28)
project(EnhancedInput.Tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

if(MSVC)
    add_compile_options(/std:c++latest /Zc:preprocessor)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-std=c++2c)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
set(ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../Engine/Source")
set(CORE_PUBLIC "${ENGINE_ROOT}/Runtime/Core/Public")
set(CORE_PRIVATE "${ENGINE_ROOT}/Runtime/Core/Private")
set(ENGINE_PUBLIC "${ENGINE_ROOT}/Runtime/Engine/Public")
set(ENGINE_PRIVATE "${ENGINE_ROOT}/Runtime/Engine/Private")
set(APPCORE_PUBLIC "${ENGINE_ROOT}/Runtime/ApplicationCore/Public")
set(EI_ROOT "${ENGINE_ROOT}/../Plugins/EnhancedInput/Source/EnhancedInput")
set(EI_PUBLIC "${EI_ROOT}/Public")
set(EI_PRIVATE "${EI_ROOT}/Private")
set(GOOGLETEST_DIR "${ENGINE_ROOT}/ThirdParty/googletest")

# ---------------------------------------------------------------------------
# Generated API headers
# ---------------------------------------------------------------------------
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/Generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")

file(WRITE "${GENERATED_DIR}/CoreAPI.generated.h"
"// Auto-generated for standalone test builds.\n"
"#pragma once\n"
"#include \"HAL/Platform.h\"\n"
"#ifdef CORE_STATIC_DEFINE\n"
"    #define CORE_API\n"
"#elif defined(CORE_EXPORTS)\n"
"    #define CORE_API DLLEXPORT\n"
"#else\n"
"    #define CORE_API DLLIMPORT\n"
"#endif\n"
)
file(WRITE "${GENERATED_DIR}/EngineAPI.generated.h"
"// Auto-generated for standalone test builds.\n"
"#pragma once\n"
"#include \"HAL/Platform.h\"\n"
"#ifdef ENGINE_STATIC_DEFINE\n"
"    #define ENGINE_API\n"
"#elif defined(ENGINE_EXPORTS)\n"
"    #define ENGINE_API DLLEXPORT\n"
"#else\n"
"    #define ENGINE_API DLLIMPORT\n"
"#endif\n"
)

file(WRITE "${GENERATED_DIR}/ApplicationCoreAPI.generated.h"
"// Auto-generated for standalone test builds.\n"
"#pragma once\n"
"#include \"HAL/Platform.h\"\n"
"#ifdef APPLICATIONCORE_STATIC_DEFINE\n"
"    #define APPLICATIONCORE_API\n"
"#elif defined(APPLICATIONCORE_EXPORTS)\n"
"    #define APPLICATIONCORE_API DLLEXPORT\n"
"#else\n"
"    #define APPLICATIONCORE_API DLLIMPORT\n"
"#endif\n"
)

# ---------------------------------------------------------------------------
# GoogleTest
# ---------------------------------------------------------------------------
add_subdirectory("${GOOGLETEST_DIR}" "${CMAKE_BINARY_DIR}/googletest")

# ---------------------------------------------------------------------------
# Core (SHARED)
# ---------------------------------------------------------------------------
set(CORE_SOURCES
    "${CORE_PRIVATE}/Modules/ModuleInitializerEntry.cpp"
    "${CORE_PRIVATE}/Modules/ModuleManager.cpp"
    "${CORE_PRIVATE}/Logging/LogSystem.cpp"
    "${CORE_PRIVATE}/Misc/AssertionMacros.cpp"
    "${CORE_PRIVATE}/Delegates/DelegateHandle.cpp"
    "${CORE_PRIVATE}/CoreModule.cpp"
)
file(GLOB CORE_MATH_SOURCES "${CORE_PRIVATE}/Math/*.cpp")

add_library(Core SHARED ${CORE_SOURCES} ${CORE_MATH_SOURCES})
target_include_directories(Core PUBLIC "${CORE_PUBLIC}" "${GENERATED_DIR}")
target_compile_definitions(Core PRIVATE CORE_EXPORTS)

# ---------------------------------------------------------------------------
# EnhancedInput sources (compiled into test exe with static defines)
# ---------------------------------------------------------------------------
set(EI_SOURCES
    "${EI_PRIVATE}/InputKeys.cpp"
    "${EI_PRIVATE}/InputActionValue.cpp"
    "${EI_PRIVATE}/InputAction.cpp"
    "${EI_PRIVATE}/InputModifiers.cpp"
    "${EI_PRIVATE}/InputTriggers.cpp"
    "${EI_PRIVATE}/InputMappingContext.cpp"
    "${EI_PRIVATE}/InputSubsystem.cpp"
    "${EI_PRIVATE}/InputMessageBridge.cpp"
)

# Engine subsystem sources
set(ENGINE_SOURCES
    "${ENGINE_PRIVATE}/Subsystems/SubsystemCollection.cpp"
)

# ---------------------------------------------------------------------------
# Test sources + stubs
# ---------------------------------------------------------------------------
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# ---------------------------------------------------------------------------
# EnhancedInputTests executable
# ---------------------------------------------------------------------------
add_executable(EnhancedInputTests
    ${TEST_SOURCES}
    ${EI_SOURCES}
    ${ENGINE_SOURCES}
)

target_include_directories(EnhancedInputTests PRIVATE
    "${CORE_PUBLIC}"
    "${ENGINE_PUBLIC}"
    "${APPCORE_PUBLIC}"
    "${EI_PUBLIC}"
    "${EI_PRIVATE}"
    "${GENERATED_DIR}"
)

target_compile_definitions(EnhancedInputTests PRIVATE
    ENGINE_STATIC_DEFINE
    APPLICATIONCORE_STATIC_DEFINE
    ENHANCEDINPUT_STATIC_DEFINE
)

target_link_libraries(EnhancedInputTests
    PRIVATE
        Core
        gtest
        gmock
        gtest_main
)

# ---------------------------------------------------------------------------
# CTest integration
# ---------------------------------------------------------------------------
include(GoogleTest)
gtest_discover_tests(EnhancedInputTests)
