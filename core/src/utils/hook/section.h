#pragma once

#include <cstdint>

namespace overlay {
namespace hook {
struct section {
    uintptr_t addr;
    uint64_t size;
};
};  // namespace hook
};  // namespace overlay