# LaunchModuleTest: verifies GuardedMain lifecycle orchestration
# main -> GuardedMain -> FEngineLoop (PreInit->Init->Tick loop->Exit)
cmake_minimum_required(VERSION 3.28)
project(LaunchModuleTest LANGUAGES CXX)

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
set(LAUNCH_PUBLIC "${ENGINE_ROOT}/Launch/Public")
set(LAUNCH_PRIVATE "${ENGINE_ROOT}/Launch/Private")

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
    "${ENGINE_PRIVATE}/Engine/GameInstance.cpp"
    "${ENGINE_PRIVATE}/EngineLoop.cpp"
)
target_include_directories(Engine PUBLIC "${ENGINE_PUBLIC}" "${CORE_PUBLIC}")
target_link_libraries(Engine PUBLIC Core)
target_compile_definitions(Engine PRIVATE ENGINE_EXPORTS)

# -- Launch (SHARED) -- GuardedMain + GEngineLoop + FLaunchModule
# NOTE: LaunchWindows.cpp (main/WinMain) is NOT included here;
#       entry points belong in the final executable, not the DLL.
add_library(Launch SHARED
    "${LAUNCH_PRIVATE}/Launch.cpp"
    "${LAUNCH_PRIVATE}/LaunchModule.cpp"
)
target_include_directories(Launch PUBLIC "${LAUNCH_PUBLIC}" "${ENGINE_PUBLIC}" "${CORE_PUBLIC}")
target_link_libraries(Launch PUBLIC Engine Core)
target_compile_definitions(Launch PRIVATE LAUNCH_EXPORTS)

# -- TestRunner --
add_executable(TestRunner Source/TestRunner/Private/Main.cpp)
target_include_directories(TestRunner PRIVATE
    "${CORE_PUBLIC}" "${ENGINE_PUBLIC}" "${LAUNCH_PUBLIC}"
)
target_link_libraries(TestRunner PRIVATE Launch Engine Core)
