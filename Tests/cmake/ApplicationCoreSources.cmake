# ApplicationCoreSources.cmake — ApplicationCore module source file list.
# Caller must set ENGINE_ROOT to <repo>/Engine/Source/Runtime before include().
#
# Variables defined:
#   APPLICATIONCORE_SOURCES — All ApplicationCore .cpp files (5 files)

if(NOT DEFINED ENGINE_ROOT)
    message(FATAL_ERROR "ENGINE_ROOT must be set before including ApplicationCoreSources.cmake")
endif()

set(_APPCORE_PRIVATE "${ENGINE_ROOT}/ApplicationCore/Private")

set(APPLICATIONCORE_SOURCES
    "${_APPCORE_PRIVATE}/ApplicationCoreModule.cpp"
    "${_APPCORE_PRIVATE}/GenericPlatform/GenericApplication.cpp"
    "${_APPCORE_PRIVATE}/Windows/WindowsWindow.cpp"
    "${_APPCORE_PRIVATE}/Windows/WindowsApplication.cpp"
    "${_APPCORE_PRIVATE}/Windows/ConsoleWindow.cpp"
)

unset(_APPCORE_PRIVATE)
