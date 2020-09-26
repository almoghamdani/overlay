#pragma once
#include <Windows.h>  // Keep this here to prevent format

#include <string>

#ifdef _WIN64
#define CORE_DLL_NAME "OverlayCore64.dll"
#else
#define CORE_DLL_NAME "OverlayCore.dll"
#endif

#define CORE_DLL_MSG_HOOK_FUNC "msg_hook"

#define MSG_POST_TIMES 5
#define MSG_POST_SLEEP 300

namespace overlay {
namespace helper {
namespace injection {

void InjectDllToThread(std::string dll, DWORD tid);

}  // namespace injection
}  // namespace helper
}  // namespace overlay