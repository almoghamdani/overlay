#pragma once

#include <cstdint>

namespace overlay {
namespace utils {
namespace hook {

class Address {
 public:
  Address() : addr_(NULL){};
  Address(uintptr_t addr) : addr_(addr){};

  template <typename T>
  operator T *() {
    return (T *)addr_;
  };

  template <typename R, typename... Args>
  R Call(Args... args) {
    return ((R(*)(Args...))addr_)(args...);
  };

 protected:
  uintptr_t addr_;
};

}  // namespace hook
}  // namespace utils
}  // namespace overlay