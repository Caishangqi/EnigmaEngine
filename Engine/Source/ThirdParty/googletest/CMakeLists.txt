# CMakeLists.txt -- GoogleTest wrapper for EnigmaEngine test projects.
# GoogleTest v1.15.2 -- https://github.com/google/googletest
#
# Usage from a test project:
#   add_subdirectory("path/to/ThirdParty/googletest" "${CMAKE_BINARY_DIR}/googletest")
#   target_link_libraries(MyTest PRIVATE gtest gmock gtest_main)

cmake_minimum_required(VERSION 3.28)

# Prevent GoogleTest from overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Disable installing GoogleTest (we only need it for building tests)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

# Build GoogleTest from source
add_subdirectory(source)
