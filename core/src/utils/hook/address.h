#pragma once

#include <cstdint>

namespace overlay {
namespace hook {
class address {
   public:
    address() : _addr(NULL){};
    address(uintptr_t addr) : _addr(addr){};

    template <typename T>
    operator T *() {
        return (T *)_addr;
    };

    template <typename R, typename... Args>
    R call(Args... args) {
        return ((R(*)(Args...))_addr)(args...);
    };

   protected:
    uintptr_t _addr;
};
};  // namespace hook
};  // namespace overlay