// Copyright EnigmaEngine. All Rights Reserved.

#include "Windows/DirectoryWatchRequestWindows.h"

#include <algorithm>
#include <cstring>

namespace Enigma
{

FDirectoryWatchRequestWindows::FDirectoryWatchRequestWindows(uint32_t InFlags)
{
    NotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME
                 | FILE_NOTIFY_CHANGE_LAST_WRITE
                 | FILE_NOTIFY_CHANGE_CREATION;

    if (InFlags & IDirectoryWatcher::IncludeDirectoryChanges)
    {
        NotifyFilter |= FILE_NOTIFY_CHANGE_DIR_NAME;
    }

    bWatchSubtree = !(InFlags & IDirectoryWatcher::IgnoreChangesInSubtree);

    Buffer.resize(BufferSize, 0);
    BackBuffer.resize(BufferSize, 0);
}

FDirectoryWatchRequestWindows::~FDirectoryWatchRequestWindows()
{
    if (DirectoryHandle != INVALID_HANDLE_VALUE)
    {
        ::CancelIoEx(DirectoryHandle, &Overlapped);
        ::WaitForSingleObjectEx(DirectoryHandle, 1000, TRUE);
        ::CloseHandle(DirectoryHandle);
        DirectoryHandle = INVALID_HANDLE_VALUE;
    }
}

bool FDirectoryWatchRequestWindows::Init(const std::string& InDirectory)
{
    Directory = InDirectory;

    DirectoryHandle = ::CreateFileA(
        Directory.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (DirectoryHandle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    // Store this pointer in hEvent for the APC callback.
    std::memset(&Overlapped, 0, sizeof(Overlapped));
    Overlapped.hEvent = reinterpret_cast<HANDLE>(this);

    // Issue the first async read.
    BOOL bSuccess = ::ReadDirectoryChangesW(
        DirectoryHandle,
        Buffer.data(),
        static_cast<DWORD>(Buffer.size()),
        bWatchSubtree ? TRUE : FALSE,
        NotifyFilter,
        nullptr,
        &Overlapped,
        &ChangeNotification);

    if (!bSuccess)
    {
        ::CloseHandle(DirectoryHandle);
        DirectoryHandle = INVALID_HANDLE_VALUE;
        return false;
    }

    return true;
}

FDelegateHandle FDirectoryWatchRequestWindows::AddDelegate(
    const IDirectoryWatcher::FDirectoryChanged& Callback)
{
    FDelegateEntry Entry;
    Entry.Handle = FDelegateHandle::Generate();
    Entry.Delegate = Callback;
    Delegates.push_back(std::move(Entry));
    return Delegates.back().Handle;
}

bool FDirectoryWatchRequestWindows::RemoveDelegate(FDelegateHandle Handle)
{
    auto It = std::remove_if(Delegates.begin(), Delegates.end(),
        [&Handle](const FDelegateEntry& E) { return E.Handle == Handle; });

    if (It != Delegates.end())
    {
        Delegates.erase(It, Delegates.end());
        return true;
    }
    return false;
}

bool FDirectoryWatchRequestWindows::HasDelegates() const
{
    return !Delegates.empty();
}

void FDirectoryWatchRequestWindows::EndWatchRequest()
{
    if (bEndWatchRequestInvoked)
    {
        return;
    }
    bEndWatchRequestInvoked = true;

    if (DirectoryHandle != INVALID_HANDLE_VALUE)
    {
        ::CancelIoEx(DirectoryHandle, &Overlapped);
        ::WaitForSingleObjectEx(DirectoryHandle, 1000, TRUE);
        ::CloseHandle(DirectoryHandle);
        DirectoryHandle = INVALID_HANDLE_VALUE;
    }

    bPendingDelete = true;
}

void FDirectoryWatchRequestWindows::ProcessPendingNotifications()
{
    if (FileChanges.empty())
    {
        return;
    }

    // Dispatch to all registered delegates.
    for (auto& Entry : Delegates)
    {
        Entry.Delegate.Execute(FileChanges);
    }

    FileChanges.clear();
}

// static
void CALLBACK FDirectoryWatchRequestWindows::ChangeNotification(
    DWORD InError, DWORD InNumBytes, LPOVERLAPPED InOverlapped)
{
    auto* Request = reinterpret_cast<FDirectoryWatchRequestWindows*>(
        InOverlapped->hEvent);
    Request->ProcessChange(InError, InNumBytes);
}

void FDirectoryWatchRequestWindows::ProcessChange(DWORD InError, DWORD InNumBytes)
{
    if (bEndWatchRequestInvoked)
    {
        return;
    }

    if (InError == ERROR_OPERATION_ABORTED)
    {
        // Watch was cancelled.
        bPendingDelete = true;
        return;
    }

    if (InError != ERROR_SUCCESS || InNumBytes == 0)
    {
        // Re-issue the read even on error.
        ::ReadDirectoryChangesW(
            DirectoryHandle,
            Buffer.data(),
            static_cast<DWORD>(Buffer.size()),
            bWatchSubtree ? TRUE : FALSE,
            NotifyFilter,
            nullptr,
            &Overlapped,
            &ChangeNotification);
        return;
    }

    // Swap buffers so we can re-issue the read immediately.
    std::swap(Buffer, BackBuffer);

    // Re-issue the read before processing (minimize missed changes).
    ::ReadDirectoryChangesW(
        DirectoryHandle,
        Buffer.data(),
        static_cast<DWORD>(Buffer.size()),
        bWatchSubtree ? TRUE : FALSE,
        NotifyFilter,
        nullptr,
        &Overlapped,
        &ChangeNotification);

    // Parse FILE_NOTIFY_INFORMATION from the back buffer.
    const uint8_t* Base = BackBuffer.data();
    for (;;)
    {
        auto* Info = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(Base);

        // Convert wide filename to narrow string.
        int NameLen = static_cast<int>(Info->FileNameLength / sizeof(WCHAR));
        int Needed = ::WideCharToMultiByte(
            CP_UTF8, 0, Info->FileName, NameLen,
            nullptr, 0, nullptr, nullptr);

        std::string Filename(static_cast<size_t>(Needed), '\0');
        ::WideCharToMultiByte(
            CP_UTF8, 0, Info->FileName, NameLen,
            Filename.data(), Needed, nullptr, nullptr);

        // Prepend directory path.
        std::string FullPath = Directory;
        if (!FullPath.empty() && FullPath.back() != '\\' && FullPath.back() != '/')
        {
            FullPath += '\\';
        }
        FullPath += Filename;

        // Map action.
        FFileChangeData::EFileChangeAction Action = FFileChangeData::FCA_Unknown;
        switch (Info->Action)
        {
        case FILE_ACTION_ADDED:
        case FILE_ACTION_RENAMED_NEW_NAME:
            Action = FFileChangeData::FCA_Added;
            break;
        case FILE_ACTION_REMOVED:
        case FILE_ACTION_RENAMED_OLD_NAME:
            Action = FFileChangeData::FCA_Removed;
            break;
        case FILE_ACTION_MODIFIED:
            Action = FFileChangeData::FCA_Modified;
            break;
        default:
            break;
        }

        FFileChangeData Change;
        Change.Filename = std::move(FullPath);
        Change.Action = Action;
        FileChanges.push_back(std::move(Change));

        if (Info->NextEntryOffset == 0)
        {
            break;
        }
        Base += Info->NextEntryOffset;
    }
}

} // namespace Enigma
