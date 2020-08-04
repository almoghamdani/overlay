#include "utils.h"

#include <Shellapi.h>
#include <TlHelp32.h>

namespace overlay {
namespace helper {
namespace utils {

DWORD GetMainThreadIdForProcess(DWORD pid) {
  THREADENTRY32 thread_entry;
  ULONGLONG min_thread_create_time = (ULONGLONG)(~(ULONGLONG)0);
  DWORD tid = 0;

  // Create snapshot of the dest process
  HANDLE thread_snapshot_handle =
      CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);

  // If the snapshot is invalid (probably process not found)
  if (thread_snapshot_handle == INVALID_HANDLE_VALUE) {
    return 0;
  }

  // Set size of structure
  thread_entry.dwSize = sizeof(THREADENTRY32);

  // For each thread in the snapshot
  for (BOOL success = Thread32First(thread_snapshot_handle, &thread_entry);
       !tid && success && GetLastError() != ERROR_NO_MORE_FILES;
       success = Thread32Next(thread_snapshot_handle, &thread_entry)) {
    // If the thread belongs to the dest process
    if (thread_entry.th32OwnerProcessID == pid) {
      // Open the thread object
      HANDLE hThread =
          OpenThread(THREAD_QUERY_INFORMATION, TRUE, thread_entry.th32ThreadID);

      if (hThread) {
        FILETIME afTimes[4] = {0};

        // Check the time the thread was created in order to find the thread who
        // started first (main thread)
        if (GetThreadTimes(hThread, &afTimes[0], &afTimes[1], &afTimes[2],
                           &afTimes[3])) {
          ULONGLONG ullTest = ((ULONGLONG(afTimes[0].dwHighDateTime) << 32) |
                               ((afTimes[0].dwLowDateTime) & 0xFFFFFFFF));
          if (ullTest && ullTest < min_thread_create_time) {
            min_thread_create_time = ullTest;
            tid = thread_entry.th32ThreadID;
          }
        }

        CloseHandle(hThread);
      }
    }
  }

  CloseHandle(thread_snapshot_handle);

  return tid;
}

}  // namespace utils
}  // namespace helper
}  // namespace overlay