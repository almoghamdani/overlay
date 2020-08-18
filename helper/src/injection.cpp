#include "injection.h"

#include <overlay/error.h>
#include <overlay/injection.h>

#include <iostream>

#include "utils.h"

namespace overlay {
namespace helper {
namespace injection {

void InjectDllToThread(std::string dll, DWORD tid) {
  // Load the core library DLL and get the proc address of the msg hook
  HMODULE lib = LoadLibraryA(dll.c_str());
  HOOKPROC proc = (HOOKPROC)GetProcAddress(lib, CORE_DLL_MSG_HOOK_FUNC);

  if (!lib) {
    throw Error(ErrorCode::CoreDllNotFound);
  } else if (!proc) {
    throw Error(ErrorCode::InvalidCoreDll);
  }

  // Set the msg hook function in the dest process (thread)
  if (!SetWindowsHookExA(WH_GETMESSAGE, proc, lib, tid)) {
    throw Error(ErrorCode::UnknownError);
  }

  // Post message for the dest thread to trigger loading the library in the dest
  // process
  for (unsigned int i = 0; i < MSG_POST_TIMES; i++) {
    if (!PostThreadMessage(tid, WM_USER + 432, 0, 0)) {
      throw Error(ErrorCode::UnknownError);
    }

    // Sleep to not spam process
    Sleep(MSG_POST_SLEEP);
  }
}

}  // namespace injection

void InjectCoreToProcess(DWORD pid) {
  DWORD tid = utils::GetMainThreadIdForProcess(pid);

  // If the dest process wasn't found
  if (tid == 0) {
    throw Error(ErrorCode::ProcessNotFound);
  }

  // Inject the core DLL to the dest process
  injection::InjectDllToThread(CORE_DLL_NAME, tid);
}

}  // namespace helper
}  // namespace overlay