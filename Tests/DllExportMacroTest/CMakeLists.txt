# DLL Export Macro Test: verifies Platform.h, CoreAPI.h, EngineAPI.h, LaunchAPI.h
cmake_minimum_required(VERSION 3.28)
project(DllExportMacroTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/std:c++latest)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-std=c++2c)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)

# Engine module public header paths
set(CORE_PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../Engine/Source/Runtime/Core/Public")

# Test executable -- compiled as a consumer (no _EXPORTS defined)
add_executable(DllExportMacroTest Source/Main.cpp)
target_include_directories(DllExportMacroTest PRIVATE "${CORE_PUBLIC}")
