# FEngineLoop test: verifies full engine lifecycle
# PreInit -> Init -> Tick -> Exit with module loading phases
cmake_minimum_required(VERSION 3.28)
project(EngineLoopTest LANGUAGES CXX)

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

set(ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../Engine/Source/Runtime")
set(CORE_PUBLIC "${ENGINE_ROOT}/Core/Public")
set(CORE_PRIVATE "${ENGINE_ROOT}/Core/Private")
set(ENGINE_PUBLIC "${ENGINE_ROOT}/Engine/Public")
set(ENGINE_PRIVATE "${ENGINE_ROOT}/Engine/Private")

# -- Core (SHARED) --
add_library(Core SHARED
    "${CORE_PRIVATE}/Modules/ModuleInitializerEntry.cpp"
    "${CORE_PRIVATE}/Modules/ModuleManager.cpp"
    "${CORE_PRIVATE}/Logging/LogSystem.cpp"
    "${CORE_PRIVATE}/Misc/AssertionMacros.cpp"
    "${CORE_PRIVATE}/CoreModule.cpp"
)
target_include_directories(Core PUBLIC "${CORE_PUBLIC}")
target_compile_definitions(Core PRIVATE CORE_EXPORTS)

# -- Engine (SHARED) --
add_library(Engine SHARED
    "${ENGINE_PRIVATE}/Engine/Engine.cpp"
    "${ENGINE_PRIVATE}/Engine/GameEngine.cpp"
    "${ENGINE_PRIVATE}/EngineLoop.cpp"
)
target_include_directories(Engine PUBLIC "${ENGINE_PUBLIC}" "${CORE_PUBLIC}")
target_link_libraries(Engine PUBLIC Core)
target_compile_definitions(Engine PRIVATE ENGINE_EXPORTS)

# -- TestEarlyModule (SHARED, loaded in EarliestPossible phase) --
add_library(TestEarlyModule SHARED
    Source/TestEarlyModule/Private/TestEarlyModule.cpp
)
target_include_directories(TestEarlyModule PRIVATE "${CORE_PUBLIC}")
target_link_libraries(TestEarlyModule PRIVATE Core)

# -- TestDefaultModule (SHARED, loaded in Default phase) --
add_library(TestDefaultModule SHARED
    Source/TestDefaultModule/Private/TestDefaultModule.cpp
)
target_include_directories(TestDefaultModule PRIVATE "${CORE_PUBLIC}")
target_link_libraries(TestDefaultModule PRIVATE Core)

# -- TestPostEngineModule (SHARED, loaded in PostEngineInit phase) --
add_library(TestPostEngineModule SHARED
    Source/TestPostEngineModule/Private/TestPostEngineModule.cpp
)
target_include_directories(TestPostEngineModule PRIVATE "${CORE_PUBLIC}")
target_link_libraries(TestPostEngineModule PRIVATE Core)

# -- TestRunner --
add_executable(TestRunner Source/TestRunner/Private/Main.cpp)
target_include_directories(TestRunner PRIVATE "${CORE_PUBLIC}" "${ENGINE_PUBLIC}")
target_link_libraries(TestRunner PRIVATE Core Engine)
