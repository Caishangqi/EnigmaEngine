# EngineSources.cmake — Engine module source file lists.
# Caller must set ENGINE_ROOT to <repo>/Engine/Source/Runtime before include().
#
# Variables defined:
#   ENGINE_FULL_SOURCES          — Complete SHARED build (15 files)
#   ENGINE_MINIMAL_SOURCES       — Minimal STATIC build (10 files, no EngineLoop/Module/Globals/GameEngine/Delegates)
#   ENGINE_REQUIRED_CORE_SOURCES — Core source sets required by Engine (Base + Config + Name + Async)

if(NOT DEFINED ENGINE_ROOT)
    message(FATAL_ERROR "ENGINE_ROOT must be set before including EngineSources.cmake")
endif()

# Include CoreSources.cmake for the CORE_* variables Engine depends on.
include("${CMAKE_CURRENT_LIST_DIR}/CoreSources.cmake")

# Core source sets that Engine module requires at link time.
# Test projects using ENGINE_*_SOURCES should include these in their Core library.
set(ENGINE_REQUIRED_CORE_SOURCES
    ${CORE_BASE_SOURCES}
    ${CORE_CONFIG_SOURCES}
    ${CORE_NAME_SOURCES}
    ${CORE_ASYNC_SOURCES}
)

set(_ENGINE_PRIVATE "${ENGINE_ROOT}/Engine/Private")

set(ENGINE_MINIMAL_SOURCES
    "${_ENGINE_PRIVATE}/Engine/Engine.cpp"
    "${_ENGINE_PRIVATE}/Subsystems/SubsystemCollection.cpp"
    "${_ENGINE_PRIVATE}/GameFramework/Component.cpp"
    "${_ENGINE_PRIVATE}/GameFramework/RenderComponent.cpp"
    "${_ENGINE_PRIVATE}/GameFramework/GameObject.cpp"
    "${_ENGINE_PRIVATE}/GameFramework/Scene.cpp"
    "${_ENGINE_PRIVATE}/GameFramework/SceneManager.cpp"
    "${_ENGINE_PRIVATE}/GameFramework/GameInstance.cpp"
    "${_ENGINE_PRIVATE}/TickSystem/TickFunction.cpp"
    "${_ENGINE_PRIVATE}/TickSystem/TickTaskManager.cpp"
)

set(ENGINE_FULL_SOURCES
    ${ENGINE_MINIMAL_SOURCES}
    "${_ENGINE_PRIVATE}/Engine/GameEngine.cpp"
    "${_ENGINE_PRIVATE}/Engine/EngineDelegates.cpp"
    "${_ENGINE_PRIVATE}/EngineLoop.cpp"
    "${_ENGINE_PRIVATE}/EngineModule.cpp"
    "${_ENGINE_PRIVATE}/EngineGlobals.cpp"
)

unset(_ENGINE_PRIVATE)
