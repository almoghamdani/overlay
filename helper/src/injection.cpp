#include "injection.h"

#include <iostream>

#include "../include/overlay_helper.h"
#include "utils.h"

namespace overlay {
namespace helper {
namespace injection {
    
bool InjectDllToThread(std::string dll, DWORD tid) {
  // Load the core library DLL and get the proc address of the msg hook
  HMODULE lib = LoadLibraryA(dll.c_str());
  HOOKPROC proc = (HOOKPROC)GetProcAddress(lib, CORE_DLL_MSG_HOOK_FUNC);

  if (!lib) {
    std::cerr << "[OverlayHelper] Unable to load core DLL!" << std::endl;
    return false;
  } else if (!proc) {
    std::cerr
        << "[OverlayHelper] Unable to get proc address of msg hook function!"
        << std::endl;
    return false;
  }

  // Set the msg hook function in the dest process (thread)
  if (!SetWindowsHookExA(WH_GETMESSAGE, proc, lib, tid)) {
    std::cerr << "[OverlayHelper] Unable to set msg hook function (Error: "
              << std::system_category().message(GetLastError()) << ")!"
              << std::endl;
    return false;
  }

  // Post message for the dest thread to trigger loading the library in the dest
  // process
  for (unsigned int i = 0; i < MSG_POST_TIMES; i++) {
    if (!PostThreadMessage(tid, WM_USER + 432, 0, 0)) {
      std::cerr << "[OverlayHelper] Unable to post message to the dest thread!"
                << std::endl;
      return false;
    }

    // Sleep to not spam process
    Sleep(MSG_POST_SLEEP);
  }

  return true;
}

}  // namespace injection
}  // namespace helper
}  // namespace overlay

bool OverlayInjectToProcess(DWORD pid) {
  DWORD tid = overlay::helper::utils::GetMainThreadIdForProcess(pid);

  // If the dest process wasn't found
  if (tid == 0) {
    std::cerr << "[OverlayHelper] Dest process wasn't found!" << std::endl;
    return false;
  }

  // Inject the core DLL to the dest process
  if (!overlay::helper::injection::InjectDllToThread(CORE_DLL_NAME, tid)) {
    std::cerr << "[OverlayHelper] Unknown error occurred!" << std::endl;
    return false;
  }

  return true;
}