#pragma once
#include <stddef.h>

namespace overlay {
namespace utils {

class Hash {
 public:
  inline static void HashCombine(size_t& s, size_t h) {
    s ^= h + 0x9e3779b9 + (s << 6) + (s >> 2);
  }

  template <class T>
  inline static void HashCombine(size_t& s, const T& v) {
    std::hash<T> h;
    HashCombine(s, h(v));
  }
};

}  // namespace utils
}  // namespace overlay