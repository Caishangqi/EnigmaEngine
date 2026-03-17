# RenderCoreSources.cmake — RenderCore module source file list.
# Caller must set ENGINE_ROOT to <repo>/Engine/Source/Runtime before include().
#
# Variables defined:
#   RENDERCORE_SOURCES — All RenderCore .cpp files (1 file)

if(NOT DEFINED ENGINE_ROOT)
    message(FATAL_ERROR "ENGINE_ROOT must be set before including RenderCoreSources.cmake")
endif()

set(RENDERCORE_SOURCES
    "${ENGINE_ROOT}/RenderCore/Private/RenderCoreModule.cpp"
)
