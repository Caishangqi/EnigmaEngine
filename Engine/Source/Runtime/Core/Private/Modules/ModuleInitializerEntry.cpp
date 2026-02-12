// Copyright EnigmaEngine. All Rights Reserved.

#include "Modules/ModuleInitializerEntry.h"
#include <cstring>

namespace Enigma
{

// Global head of the self-registering module linked list.
// Each DLL's static FModuleInitializerEntry prepends itself here on load.
static FModuleInitializerEntry* GFirstModuleInitializerEntry = nullptr;

FModuleInitializerEntry::FModuleInitializerEntry(
    const char* InName,
    FInitializeModuleFunctionPtr InFunction)
    : Prev(nullptr)
    , Next(GFirstModuleInitializerEntry)
    , Name(InName)
    , Function(InFunction)
{
    if (GFirstModuleInitializerEntry)
    {
        GFirstModuleInitializerEntry->Prev = this;
    }
    GFirstModuleInitializerEntry = this;
}

FModuleInitializerEntry::~FModuleInitializerEntry()
{
    if (Next)
    {
        Next->Prev = Prev;
    }

    if (Prev)
    {
        Prev->Next = Next;
    }
    else
    {
        GFirstModuleInitializerEntry = Next;
    }
}

FInitializeModuleFunctionPtr FModuleInitializerEntry::FindModule(const char* InName)
{
    for (auto* Entry = GFirstModuleInitializerEntry; Entry; Entry = Entry->Next)
    {
        if (std::strcmp(InName, Entry->Name) == 0)
        {
            return Entry->Function;
        }
    }
    return nullptr;
}

void FModuleInitializerEntry::ForEach(ForEachCallback Callback, void* UserData)
{
    for (auto* Entry = GFirstModuleInitializerEntry; Entry; Entry = Entry->Next)
    {
        Callback(Entry->Name, UserData);
    }
}

} // namespace Enigma
