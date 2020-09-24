#include "hook.h"

#include <MinHook.h>

namespace overlay {
namespace utils {
namespace hook {

bool Hook::Install(void *src, void *dst) {
  src_ = src;

  return MH_CreateHook(src, dst, &trampoline_) == MH_OK &&
         MH_EnableHook(src) == MH_OK;
}

bool Hook::Remove() { return MH_RemoveHook(src_) == MH_OK; }

Address Hook::get_trampoline() const { return Address((uintptr_t)trampoline_); }

}  // namespace hook
}  // namespace utils
}  // namespace overlay