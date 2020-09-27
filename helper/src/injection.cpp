#include <overlay/error.h>
#include <overlay/injection.h>

#include <sstream>
#include <string>

typedef BOOL(WINAPI *pFnIsWow64Process)(HANDLE, PBOOL);
typedef void(WINAPI *pFnGetNativeSystemInfo)(LPSYSTEM_INFO);

#define INJECTOR32_NAME "OverlayInjector.exe"
#define INJECTOR64_NAME "OverlayInjector64.exe"

#define PROCESS_NOT_FOUND_ERROR_CODE -1
#define DLL_NOT_FOUND_ERROR_CODE -2
#define INVALID_DLL_ERROR_CODE -3
#define UNKNOWN_ERROR_CODE -4

namespace overlay {
namespace helper {

bool IsProcess32Bit(HANDLE process_handle) {
  pFnIsWow64Process is_wow_64_process_fn = (pFnIsWow64Process)GetProcAddress(
      GetModuleHandleA("kernel32.dll"), "IsWow64Process");
  BOOL is_wow_64_process_ret = FALSE;

  pFnGetNativeSystemInfo get_native_system_info_fn =
      (pFnGetNativeSystemInfo)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                             "GetNativeSystemInfo");
  SYSTEM_INFO system_info;

  if (is_wow_64_process_fn) {
    is_wow_64_process_fn(process_handle, &is_wow_64_process_ret);
    if (is_wow_64_process_ret == TRUE) {  // Process is running on WoW64
      return true;
    }
  }

  if (get_native_system_info_fn) {
    get_native_system_info_fn(&system_info);
  } else {
    GetSystemInfo(&system_info);
  }

  return system_info.dwProcessorType == PROCESSOR_ARCHITECTURE_INTEL;
}

std::string FormatInjectorCommandLineArgs(DWORD pid, bool process_32_bit) {
  std::stringstream ss;

  ss << (process_32_bit ? INJECTOR32_NAME : INJECTOR64_NAME) << " " << pid;

  return ss.str();
}

void InjectCoreToProcess(DWORD pid) {
  STARTUPINFOA injector_startup_info = {sizeof(injector_startup_info)};
  PROCESS_INFORMATION injector_process_info = {0};
  DWORD injector_exit_code = 0;

  // Open the dest process
  HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION, false, pid);
  if (!process_handle) {
    throw Error(ErrorCode::ProcessNotFound);
  }

  // Start the injector process
  if (!CreateProcessA(NULL,
                      (LPSTR)FormatInjectorCommandLineArgs(
                          pid, IsProcess32Bit(process_handle))
                          .c_str(),
                      NULL, NULL, FALSE, 0, NULL, NULL, &injector_startup_info,
                      &injector_process_info)) {
    throw Error(ErrorCode::InjectorNotFound);
  }

  // Wait for the injector to exit
  WaitForSingleObject(injector_process_info.hProcess, INFINITE);

  // Get the exit code of the injector
  GetExitCodeProcess(injector_process_info.hProcess, &injector_exit_code);

  // Close all handles
  CloseHandle(process_handle);
  CloseHandle(injector_process_info.hProcess);
  CloseHandle(injector_process_info.hThread);

  // Check for errors
  switch ((int)injector_exit_code) {
    case 0:
      return;

    case PROCESS_NOT_FOUND_ERROR_CODE:
      throw Error(ErrorCode::ProcessNotFound);

    case DLL_NOT_FOUND_ERROR_CODE:
      throw Error(ErrorCode::CoreDllNotFound);

    case INVALID_DLL_ERROR_CODE:
      throw Error(ErrorCode::InvalidCoreDll);

    default:
    case UNKNOWN_ERROR_CODE:
      throw Error(ErrorCode::UnknownError);
  }
}

}  // namespace helper
}  // namespace overlay