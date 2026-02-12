// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"

namespace Enigma
{

class IModuleInterface;

// Function pointer type for module initializer functions.
// Returns a new instance of the module's IModuleInterface implementation.
using FInitializeModuleFunctionPtr = IModuleInterface* (*)();

// Intrusive linked list node for self-registering module initializers.
//
// When a DLL is loaded, static FModuleInitializerEntry instances
// (created by IMPLEMENT_MODULE) register themselves into a global
// linked list. The ModuleManager can then find any loaded module's
// initializer by name without using GetProcAddress/dlsym.
//
// This mirrors Unreal Engine's module registration pattern.
class CORE_API FModuleInitializerEntry
{
public:
    FModuleInitializerEntry(const char* InName, FInitializeModuleFunctionPtr InFunction);
    ~FModuleInitializerEntry();

    // Non-copyable, non-movable
    FModuleInitializerEntry(const FModuleInitializerEntry&) = delete;
    FModuleInitializerEntry& operator=(const FModuleInitializerEntry&) = delete;

    // Find a module initializer by name. Returns nullptr if not found.
    static FInitializeModuleFunctionPtr FindModule(const char* Name);

    // Iterate all registered entries, calling the callback with each module name.
    using ForEachCallback = void (*)(const char* Name, void* UserData);
    static void ForEach(ForEachCallback Callback, void* UserData = nullptr);

private:
    FModuleInitializerEntry* Prev;
    FModuleInitializerEntry* Next;
    const char* Name;
    FInitializeModuleFunctionPtr Function;
};

} // namespace Enigma
