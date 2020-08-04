#include <Windows.h>
#include <psapi.h>

#include <loguru.hpp>
#include <string>
#include <thread>

#include "core.h"

#define HELPER_EXE_NAME "OverlayHelper"

bool IsLoadedByHelper() {
  HMODULE modules[1024];
  DWORD size;

  // Get a list of all the modules in this process.
  if (EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules),
                         &size)) {
    for (int i = 0; i < (size / sizeof(HMODULE)); i++) {
      TCHAR moduleName[MAX_PATH];

      // Get the full path to the module's file.
      if (GetModuleFileNameEx(GetCurrentProcess(), modules[i], moduleName,
                              sizeof(moduleName) / sizeof(TCHAR))) {
        // Check if the module name contains helper name
        if (std::string((const char *)moduleName).find(HELPER_EXE_NAME) !=
            std::string::npos) {
          return true;
        }
      }
    }
  }

  return false;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  // If the library was loaded by the helper, do nothing
  if (IsLoadedByHelper()) {
    return true;
  }

  // If the process was just attached to this library
  if (fdwReason == DLL_PROCESS_ATTACH) {
    // Load the library in the target process (DLL Injection)
    TCHAR dllName[MAX_PATH];
    GetModuleFileName(hinstDLL, dllName, MAX_PATH);
    LoadLibrary(dllName);

#ifdef DEBUG
    // Create debug console and redirect stdin, stdout and stderr to it
    AllocConsole();
    SetConsoleTitleW(L"Debug Console");
    freopen("conin$", "r+t", stdin);
    freopen("conout$", "w+t", stdout);
    freopen("conout$", "w+t", stderr);
#endif

    // Disable calling DllMain for DLL_THREAD_ATTACH and DLL_THREAD_DETACH
    DisableThreadLibraryCalls((HMODULE)hinstDLL);

    LOG_F(INFO, "Overlay Core v1.0\n");

    // Start overlay core
    overlay::core::Core::Get()->Start();
  }

  return true;
}