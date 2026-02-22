# Task 5.1 Test: nlohmann/json ThirdParty header-only integration
# Validates: #include <nlohmann/json.hpp> works from a dependent module
cmake_minimum_required(VERSION 3.28)
project(NlohmannJsonTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/std:c++latest)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-std=c++2c)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)

# nlohmann/json header-only include path
set(NLOHMANN_JSON_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../../Engine/Source/ThirdParty/nlohmann_json/include")

# -- TestRunner --
add_executable(TestRunner Source/Main.cpp)
target_include_directories(TestRunner PRIVATE "${NLOHMANN_JSON_INCLUDE}")

enable_testing()
add_test(NAME TestRunner COMMAND $<TARGET_FILE:TestRunner>)
