# CoreModule test: verifies Core.dll builds, loads via FModuleManager,
# StartupModule/ShutdownModule lifecycle, CoreMinimal.h unified include
cmake_minimum_required(VERSION 3.28)
project(CoreModuleTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/std:c++latest)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-std=c++2c)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)

set(CORE_PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../Engine/Source/Runtime/Core/Public")
set(CORE_PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../Engine/Source/Runtime/Core/Private")

# -- Core (SHARED) -- the module under test
add_library(Core SHARED
    "${CORE_PRIVATE}/Modules/ModuleInitializerEntry.cpp"
    "${CORE_PRIVATE}/Modules/ModuleManager.cpp"
    "${CORE_PRIVATE}/Logging/LogSystem.cpp"
    "${CORE_PRIVATE}/Misc/AssertionMacros.cpp"
    "${CORE_PRIVATE}/CoreModule.cpp"
)
target_include_directories(Core PUBLIC "${CORE_PUBLIC}")
target_compile_definitions(Core PRIVATE CORE_EXPORTS)

# -- TestRunner (executable, loads Core via FModuleManager) --
add_executable(TestRunner Source/Main.cpp)
target_include_directories(TestRunner PRIVATE "${CORE_PUBLIC}")
target_link_libraries(TestRunner PRIVATE Core)
