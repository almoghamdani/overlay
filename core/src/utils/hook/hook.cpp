#include "hook.h"

namespace overlay {
namespace utils {
namespace hook {

bool Hook::Install(void *src, void *dst) {
  // Try to install the hook
#ifdef _WIN64
  return hook_.Install(src, dst, subhook::HookFlag64BitOffset);
#else
  return hook_.Install(src, dst);
#endif
}

Address Hook::get_trampoline() const {
  return Address((uintptr_t)hook_.GetTrampoline());
}

}  // namespace hook
}  // namespace utils
}  // namespace overlay