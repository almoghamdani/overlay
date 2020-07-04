#include "utils.h"

#include <Shellapi.h>
#include <TlHelp32.h>

DWORD overlay::helper::utils::get_main_thread_id_for_process(DWORD pid) {
    THREADENTRY32 tEntry;
    ULONGLONG ullMinCreateTime = (ULONGLONG)(~(ULONGLONG)0);
    DWORD tid = 0;

    // Create snapshot of the dest process
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);

    // If the snapshot is invalid (probably process not found)
    if (hThreadSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    // Set size of structure
    tEntry.dwSize = sizeof(THREADENTRY32);

    // For each thread in the snapshot
    for (BOOL success = Thread32First(hThreadSnapshot, &tEntry);
         !tid && success && GetLastError() != ERROR_NO_MORE_FILES;
         success = Thread32Next(hThreadSnapshot, &tEntry)) {
        // If the thread belongs to the dest process
        if (tEntry.th32OwnerProcessID == pid) {
            // Open the thread object
            HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, TRUE, tEntry.th32ThreadID);

            if (hThread) {
                FILETIME afTimes[4] = {0};

                // Check the time the thread was created in order to find the thread who started
                // first (main thread)
                if (GetThreadTimes(hThread, &afTimes[0], &afTimes[1], &afTimes[2], &afTimes[3])) {
                    ULONGLONG ullTest = ((ULONGLONG(afTimes[0].dwHighDateTime) << 32) |
                                         ((afTimes[0].dwLowDateTime) & 0xFFFFFFFF));
                    if (ullTest && ullTest < ullMinCreateTime) {
                        ullMinCreateTime = ullTest;
                        tid = tEntry.th32ThreadID;
                    }
                }

                CloseHandle(hThread);
            }
        }
    }

    CloseHandle(hThreadSnapshot);

    return tid;
}