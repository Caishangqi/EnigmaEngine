// Copyright EnigmaEngine. All Rights Reserved.
// Logging system implementation (REQ-016)

#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Logging/LogVerbosity.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace Enigma
{

// -- FLogCategoryBase ----------------------------------------

FLogCategoryBase::FLogCategoryBase(
    const char* InName,
    ELogVerbosity InDefaultVerbosity,
    ELogVerbosity InCompileTimeVerbosity) noexcept
    : Name(InName)
    , RuntimeVerbosity(InDefaultVerbosity)
    , DefaultVerbosity(InDefaultVerbosity)
    , CompileTimeVerbosity(InCompileTimeVerbosity)
{
}

void FLogCategoryBase::SetVerbosity(ELogVerbosity NewVerbosity) noexcept
{
    // Clamp: never allow more verbose than compile-time limit
    if (static_cast<uint8_t>(NewVerbosity) > static_cast<uint8_t>(CompileTimeVerbosity))
    {
        NewVerbosity = CompileTimeVerbosity;
    }
    RuntimeVerbosity = NewVerbosity;
}

// -- Output functions ----------------------------------------

namespace Logging::Private
{

void LogOutput(
    const char* CategoryName,
    ELogVerbosity Level,
    const std::string& Message) noexcept
{
    // Format: [CategoryName] Level: Message\n
    // Error and above go to stderr; Info and below go to stdout.
    FILE* stream = (static_cast<uint8_t>(Level) <= static_cast<uint8_t>(ELogVerbosity::Error))
                       ? stderr
                       : stdout;

    std::fprintf(stream, "[%s] %s: %s\n",
                 CategoryName,
                 LogVerbosityToString(Level),
                 Message.c_str());
    std::fflush(stream);
}

[[noreturn]] void LogFatal(
    const char* CategoryName,
    const std::string& Message) noexcept
{
    std::fprintf(stderr, "[%s] Fatal: %s\n", CategoryName, Message.c_str());
    std::fflush(stderr);
    std::abort();
}

} // namespace Logging::Private

} // namespace Enigma
