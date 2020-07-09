#pragma once
#include <stdint.h>

namespace overlay {
namespace hook {
class vtable {
   public:
    static void *get_function_pointer(void *obj, uint64_t index);
};
};  // namespace hook
};  // namespace overlay