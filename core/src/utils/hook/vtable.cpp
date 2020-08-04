#include "vtable.h"

namespace overlay {
namespace utils {
namespace hook {

void *Vtable::GetFunctionPointer(void *obj, uint64_t index) {
  void *vAddr = nullptr;

  if (obj) {
    uintptr_t *vtablePtr = reinterpret_cast<uintptr_t *>(*(uintptr_t *)obj);

    if (vtablePtr) {
      vAddr = reinterpret_cast<void *>(*(vtablePtr + index));
    }
  }

  return vAddr;
}

}  // namespace hook
}  // namespace utils
}  // namespace overlay