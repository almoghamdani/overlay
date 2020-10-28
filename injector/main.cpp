#include <windows.h>  // Keep this here
#include <Shellapi.h>
#include <TlHelp32.h>

#include <string>

#define PROCESS_NOT_FOUND_ERROR_CODE -1
#define DLL_NOT_FOUND_ERROR_CODE -2
#define INVALID_DLL_ERROR_CODE -3
#define UNKNOWN_ERROR_CODE -4

#ifdef _WIN64
#define CORE_DLL_NAME "OverlayCore64.dll"
#else
#define CORE_DLL_NAME "OverlayCore.dll"
#endif

#define CORE_DLL_MSG_HOOK_FUNC "msg_hook"

#define MSG_POST_TIMES 5
#define MSG_POST_SLEEP 300

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

int InjectDllToThread(std::string dll, DWORD tid) {
  // Load the core library DLL and get the proc address of the msg hook
  HMODULE lib = LoadLibraryA(dll.c_str());
  HOOKPROC proc = (HOOKPROC)GetProcAddress(lib, CORE_DLL_MSG_HOOK_FUNC);

  if (!lib) {
    return DLL_NOT_FOUND_ERROR_CODE;
  } else if (!proc) {
    return INVALID_DLL_ERROR_CODE;
  }

  // Set the msg hook function in the dest process (thread)
  if (!SetWindowsHookExW(WH_GETMESSAGE, proc, lib, tid)) {
    return UNKNOWN_ERROR_CODE;
  }

  // Post message for the dest thread to trigger loading the library in the dest
  // process
  for (unsigned int i = 0; i < MSG_POST_TIMES; i++) {
    if (!PostThreadMessage(tid, WM_USER + 432, 0, 0)) {
      return UNKNOWN_ERROR_CODE;
    }

    // Sleep to not spam process
    Sleep(MSG_POST_SLEEP);
  }

  return 0;
}

int main(int argc, char **argv) {
  DWORD pid = 0;
  DWORD tid = 0;

  // Verify process id was entered
  if (argc < 2) {
    return PROCESS_NOT_FOUND_ERROR_CODE;
  }

  try {
    // Try to convert the pid argument
    pid = std::stoi(argv[1]);
  } catch (...) {
    return PROCESS_NOT_FOUND_ERROR_CODE;
  }

  // Get the thread id for the process
  tid = GetMainThreadIdForProcess(pid);
  if (tid == 0) {
    return PROCESS_NOT_FOUND_ERROR_CODE;
  }

  return InjectDllToThread(CORE_DLL_NAME, tid);
}