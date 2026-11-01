#pragma once
#include <windows.h>

static bool labPresenceSignaled(const wchar_t* name) {
    HANDLE presence=OpenEventW(SYNCHRONIZE,FALSE,name);
    if(!presence) return false;
    const bool available=WaitForSingleObject(presence,0)==WAIT_OBJECT_0;
    CloseHandle(presence);return available;
}
static bool labAppAvailable(const wchar_t* presenceName=L"Local\\labplus.commands.Active",const wchar_t* ownerName=L"Local\\LABPlus.App.Owner") {
    if(!labPresenceSignaled(presenceName)) return false;
    HANDLE owner=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,ownerName);
    if(!owner) return false;
    const DWORD status=WaitForSingleObject(owner,0);
    // The companion UI thread owns this mutex throughout its lifetime.
    // An unowned/abandoned object is never proof of a live companion, even if
    // an old reader still holds its signaled event after a crash.
    if(status==WAIT_OBJECT_0||status==WAIT_ABANDONED) ReleaseMutex(owner);
    CloseHandle(owner);return status==WAIT_TIMEOUT;
}
