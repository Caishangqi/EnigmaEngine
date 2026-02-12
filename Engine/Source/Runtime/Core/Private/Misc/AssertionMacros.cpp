// Copyright EnigmaEngine. All Rights Reserved.
// Assertion failure handler implementation (REQ-017)

#include "Misc/AssertionMacros.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace Enigma::Assert::Private
{

[[noreturn]] void CheckFailure(
    const char* Expression,
    const char* File,
    int32_t Line,
    const std::string& Message) noexcept
{
    if (Message.empty())
    {
        std::fprintf(stderr,
            "[Fatal] Assertion failed: %s [%s:%d]\n",
            Expression, File, Line);
    }
    else
    {
        std::fprintf(stderr,
            "[Fatal] Assertion failed: %s [%s:%d] %s\n",
            Expression, File, Line, Message.c_str());
    }
    std::fflush(stderr);
    std::abort();
}

bool EnsureFailure(
    const char* Expression,
    const char* File,
    int32_t Line,
    const std::string& Message) noexcept
{
    if (Message.empty())
    {
        std::fprintf(stderr,
            "[Warning] Ensure failed: %s [%s:%d]\n",
            Expression, File, Line);
    }
    else
    {
        std::fprintf(stderr,
            "[Warning] Ensure failed: %s [%s:%d] %s\n",
            Expression, File, Line, Message.c_str());
    }
    std::fflush(stderr);
    return false;
}

} // namespace Enigma::Assert::Private
