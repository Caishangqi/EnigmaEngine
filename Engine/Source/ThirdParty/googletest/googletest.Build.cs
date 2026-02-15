// googletest.Build.cs -- Build rules for Google Test + Google Mock.
// GoogleTest v1.15.2 -- https://github.com/google/googletest
// License: BSD-3-Clause (see source/LICENSE in this directory)
//
// This is a compiled ThirdParty module providing gtest and gmock static
// libraries. The test projects link against these via CMake add_subdirectory.
// Engine modules should NOT depend on this module; it is for test targets only.
//
// Usage from a standalone CMake test project:
//   set(GOOGLETEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Engine/Source/ThirdParty/googletest")
//   add_subdirectory("${GOOGLETEST_DIR}/source" "${CMAKE_BINARY_DIR}/googletest")
//   target_link_libraries(MyTest PRIVATE gtest gmock gtest_main)

using EnigmaEngine;

public class googletest : ModuleRules
{
    public googletest()
    {
        Type = ModuleType.DeveloperTool;

        // Expose gtest and gmock public headers so that dependents
        // can write #include <gtest/gtest.h> and #include <gmock/gmock.h>
        PublicIncludePaths.Add("source/googletest/include");
        PublicIncludePaths.Add("source/googlemock/include");
    }
}
