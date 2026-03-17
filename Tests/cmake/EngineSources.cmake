# EngineSources.cmake — Engine module source file lists.
# Caller must set ENGINE_ROOT to <repo>/Engine/Source/Runtime before include().
#
# Variables defined:
#   ENGINE_FULL_SOURCES    — Complete SHARED build (13 files)
#   ENGINE_MINIMAL_SOURCES — Minimal STATIC build (8 files, no EngineLoop/Module/Globals/GameEngine/Delegates)

if(NOT DEFINED ENGINE_ROOT)
    message(FATAL_ERROR "ENGINE_ROOT must be set before including EngineSources.cmake")
endif()

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
