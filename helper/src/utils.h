#pragma once
#include <Windows.h>  // Keep this here to prevent format

namespace overlay {
namespace helper {
namespace utils {
DWORD get_main_thread_id_for_process(DWORD pid);
};
};  // namespace helper
};  // namespace overlay