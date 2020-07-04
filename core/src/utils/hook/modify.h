#pragma once

#include <cstdint>
#include <vector>

namespace overlay {
namespace hook {
class modify {
   public:
    template <typename T>
    static void put(void *dst, T opcode, uint64_t amount) {
        for (int i = 0; i < amount; i++) {
            *((T *)dst + i) = opcode;
        }
    }

    template <typename T>
    static void put(void *dst, T opcode) {
        *(T *)dst = opcode;
    }

    template <typename T>
    static void put(void *dst, std::vector<T> vec) {
        for (auto iterator = vec.begin(); iterator != vec.end(); iterator++) {
            *((T *)dst + (iterator - vec.begin())) = *iterator;
        }
    }

    static void nop(void *dst);
    static void nop(void *dst, uint64_t amount);

    static void ret(void *dst, uint8_t params_size = 0);
};
};  // namespace hook
};  // namespace overlay