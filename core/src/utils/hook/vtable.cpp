#include "vtable.h"

void *overlay::hook::vtable::get_function_pointer(void *obj, uint64_t index) {
    void *vAddr = nullptr;

    if (obj) {
        uintptr_t *vtablePtr = reinterpret_cast<uintptr_t *>(*(uintptr_t *)obj);

        if (vtablePtr) {
            vAddr = reinterpret_cast<void *>(*(vtablePtr + index));
        }
    }

    return vAddr;
}