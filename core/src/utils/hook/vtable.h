#pragma once
#include <stdint.h>

namespace overlay {
namespace utils {
namespace hook {

class Vtable {
 public:
  static void *GetFunctionPointer(void *obj, uint64_t index);
};

}  // namespace hook
}  // namespace utils
}  // namespace overlay