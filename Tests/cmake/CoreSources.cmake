# CoreSources.cmake — Composable Core module source file lists.
# Caller must set ENGINE_ROOT to <repo>/Engine/Source/Runtime before include().
#
# Variables defined:
#   CORE_BASE_SOURCES    — Minimal Core (modules, logging, assertions, CoreModule)
#   CORE_CONFIG_SOURCES  — Config system (CoreGlobals, ConfigCacheIni, ConfigDelegates)
#   CORE_NAME_SOURCES    — FName support (Name.cpp)
#   CORE_DELEGATE_SOURCES — Delegate handle (DelegateHandle.cpp)
#   CORE_MATH_SOURCES    — All Math/*.cpp via file(GLOB)
#   CORE_ASYNC_SOURCES   — Async system (ThreadPool, TaskGraph)

if(NOT DEFINED ENGINE_ROOT)
    message(FATAL_ERROR "ENGINE_ROOT must be set before including CoreSources.cmake")
endif()

set(_CORE_PRIVATE "${ENGINE_ROOT}/Core/Private")

set(CORE_BASE_SOURCES
    "${_CORE_PRIVATE}/Modules/ModuleInitializerEntry.cpp"
    "${_CORE_PRIVATE}/Modules/ModuleManager.cpp"
    "${_CORE_PRIVATE}/Logging/LogSystem.cpp"
    "${_CORE_PRIVATE}/Misc/AssertionMacros.cpp"
    "${_CORE_PRIVATE}/CoreModule.cpp"
)

set(CORE_CONFIG_SOURCES
    "${_CORE_PRIVATE}/Misc/CoreGlobals.cpp"
    "${_CORE_PRIVATE}/Misc/ConfigCacheIni.cpp"
    "${_CORE_PRIVATE}/Misc/ConfigDelegates.cpp"
)

set(CORE_NAME_SOURCES
    "${_CORE_PRIVATE}/Misc/Name.cpp"
)

set(CORE_DELEGATE_SOURCES
    "${_CORE_PRIVATE}/Delegates/DelegateHandle.cpp"
)

file(GLOB CORE_MATH_SOURCES "${_CORE_PRIVATE}/Math/*.cpp")

set(CORE_ASYNC_SOURCES
    "${_CORE_PRIVATE}/Async/ThreadPool.cpp"
    "${_CORE_PRIVATE}/Async/TaskGraph.cpp"
)

unset(_CORE_PRIVATE)
