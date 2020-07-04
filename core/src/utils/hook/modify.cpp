#include "modify.h"

#include <cstring>

void overlay::hook::modify::nop(void *dst) { put<uint8_t>(dst, 0x90); }

void overlay::hook::modify::nop(void *dst, uint64_t amount) { put<uint8_t>(dst, 0x90, amount); }

void overlay::hook::modify::ret(void *dst, uint8_t params_size) {
    if (!params_size) {
        put<uint8_t>(dst, 0xC3);
    } else {
        put<uint8_t>(dst, 0xC2);
        put<uint8_t>((char *)dst + 1, params_size);
    }
}