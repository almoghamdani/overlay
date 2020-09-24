#pragma once
#include "address.h"

namespace overlay {
namespace utils {
namespace hook {

class Hook {
 public:
  bool Install(void *src, void *dst);
  bool Remove();

  Address get_trampoline() const;

 private:
  void *src_;
  void *trampoline_;
};

}  // namespace hook
}  // namespace utils
}  // namespace overlay