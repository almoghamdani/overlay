#pragma once
#include <subhook.h>

#include <memory>

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
  subhook::Hook hook_;
};

}  // namespace hook
}  // namespace utils
}  // namespace overlay