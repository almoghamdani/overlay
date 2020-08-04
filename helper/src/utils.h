#pragma once
#include <Windows.h>  // Keep this here to prevent format

namespace overlay {
namespace helper {
namespace utils {

DWORD GetMainThreadIdForProcess(DWORD pid);

}  // namespace utils
}  // namespace helper
}  // namespace overlay