// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "Delegates/Delegate.h"
#include "Delegates/DelegateHandle.h"

#include <cstdint>
#include <string>
#include <vector>

// -------------------------------------------------------------
// IDirectoryWatcher -- Abstract directory watcher interface.
//
// Provides file system monitoring via platform-specific backends.
// Matches UE's IDirectoryWatcher structure and naming.
// -------------------------------------------------------------

namespace Enigma
{

/// Describes a single file change event.
struct FFileChangeData
{
    enum EFileChangeAction
    {
        FCA_Unknown,
        FCA_Added,
        FCA_Modified,
        FCA_Removed,
    };

    std::string         Filename;
    EFileChangeAction   Action = FCA_Unknown;
};

/// Abstract directory watcher interface.
class IDirectoryWatcher
{
public:
    enum WatchOptions : uint32_t
    {
        IncludeDirectoryChanges = (1 << 0),
        IgnoreChangesInSubtree  = (1 << 1),
    };

    using FDirectoryChanged = TDelegate<void(const std::vector<FFileChangeData>&)>;

    virtual ~IDirectoryWatcher() = default;

    virtual bool RegisterDirectoryChangedCallback(
        const std::string& Directory,
        const FDirectoryChanged& Callback,
        FDelegateHandle& OutHandle,
        uint32_t Flags = 0) = 0;

    virtual bool UnregisterDirectoryChangedCallback(
        const std::string& Directory,
        FDelegateHandle Handle) = 0;

    virtual void Tick(float DeltaSeconds) = 0;
};

} // namespace Enigma
