#pragma once
#include <cstdint>

namespace overlay {
namespace core {
namespace graphics {

struct Rect {
  uint32_t height, width;
  int32_t x, y;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay