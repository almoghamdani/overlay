#include "pattern.h"

#include <intrin.h>

#include "manager.h"

overlay::hook::pattern::pattern(std::string pattern) {
    // Create tha value and the mask from the pattern and search for it
    create_value(pattern);
    create_mask(pattern);
    search();

    // If no matches found, throw error
    if (_matches.size() == 0) {
        throw std::runtime_error("No matches were found for the given pattern.");
    }

    // Set the address if found a match
    _addr = _matches[0];
}

uintptr_t overlay::hook::pattern::get() { return _matches[0]; }

std::vector<uintptr_t> overlay::hook::pattern::get_matches() { return _matches; }

void overlay::hook::pattern::search() {
    bool avx = false, sse = false;

    __m256i avx_value, avx_mask;
    __m128i sse_value, sse_mask;

    std::vector<section> sections = manager::get_sections();

    // 256-bit
    if (_size <= 32 && is_avx_supported()) {
        avx = true;

        avx_value = _mm256_load_si256((const __m256i *)_value.data());
        avx_mask = _mm256_load_si256((const __m256i *)_mask.data());
    }
    // 128-bit
    else if (_size <= 16 && is_sse_supported()) {
        sse = true;

        sse_value = _mm_load_si128((const __m128i *)_value.data());
        sse_mask = _mm_load_si128((const __m128i *)_mask.data());
    } else {
    }

    // For each section, try to find the pattern
    for (section &section : sections) {
        uintptr_t max_address = (uintptr_t)(section.addr + section.size - _mask.size());

        // Go through the section
        for (uintptr_t section_ptr = section.addr; section_ptr < max_address; section_ptr++) {
            if (avx) {
                search_avx(avx_value, avx_mask, section_ptr);
            } else if (sse) {
                search_sse(sse_value, sse_mask, section_ptr);
            }
        }
    }
}

void overlay::hook::pattern::search_avx(__m256i &value, __m256i &mask, uintptr_t data) {
    // Load the data
    __m256i content = _mm256_loadu_si256((__m256i *)data);

    // Compare the content and the value
    __m256i compare = _mm256_cmpeq_epi8(content, value);

    // Mask the result
    __m256i and = _mm256_and_si256(compare, mask);

    // Xor between the result and the mask
    __m256i xor = _mm256_xor_si256(and, mask);

    if (_mm256_test_all_zeros(xor, xor)) {
        _matches.push_back(data);
    }
}

void overlay::hook::pattern::search_sse(__m128i &value, __m128i &mask, uintptr_t data) {
    // Load the data
    __m128i content = _mm_loadu_si128((__m128i *)data);

    // Compare the content and the value
    __m128i compare = _mm_cmpeq_epi8(content, value);

    // Mask the result
    __m128i and = _mm_and_si128(compare, mask);

    // Xor between the result and the mask
    __m128i xor = _mm_xor_si128(and, mask);

    if (_mm_test_all_zeros(xor, xor)) {
        _matches.push_back(data);
    }
}

void overlay::hook::pattern::create_value(std::string pattern) {
    // For each character in the string
    for (auto &ch = pattern.begin(); ch != pattern.end(); ch++) {
        if (*ch == '?')  // If variable, set 0 in the value
        {
            _value.push_back(0);
        } else if (isalnum(*ch)) {
            // Convert to uint16_t and push to value
            auto ch1 = *ch;
            auto ch2 = *(++ch);
            char str[] = {ch1, ch2};
            uint8_t word = (uint8_t)strtol(str, nullptr, 16);

            _value.push_back(word);
        }
    }

    // Set the size of the pattern
    _size = (uint8_t)_value.size();

    // Pad value if needed
    if (_value.size() % 32 != 0) {
        pad(_value, 32 - (_value.size() % 32));
    }
}

void overlay::hook::pattern::create_mask(std::string pattern) {
    // For each character in the string
    for (auto &ch = pattern.begin(); ch != pattern.end(); ch++) {
        if (*ch == '?')  // If variable, set 0 in the mask
        {
            _mask.push_back(0);
        } else if (isalnum(*ch)) {
            ch++;

            _mask.push_back(0xff);
        }
    }

    // Pad mask if needed
    if (_mask.size() % 32 != 0) {
        pad(_mask, 32 - (_mask.size() % 32));
    }
}

void overlay::hook::pattern::pad(std::vector<uint8_t> &value, uint16_t amount) {
    for (int i = 0; i < amount; i++) {
        value.push_back(0);
    }
}

bool overlay::hook::pattern::is_avx_supported() {
    bool avx_supported = false;
    bool osxsave_supported = false;

    int cpuinfo[4];
    __cpuid(cpuinfo, 1);

    avx_supported = cpuinfo[2] & (1 << 28) || false;
    osxsave_supported = cpuinfo[2] & (1 << 27) || false;
    if (osxsave_supported && avx_supported) {
        // _XCR_XFEATURE_ENABLED_MASK = 0
        unsigned long long xcr_feature_mask = _xgetbv(0);
        avx_supported = (xcr_feature_mask & 0x6) == 0x6;
    }

    return avx_supported;
}

bool overlay::hook::pattern::is_sse_supported() {
    int cpuinfo[4];
    __cpuid(cpuinfo, 1);

    return cpuinfo[2] & (1 << 20) || false;
}