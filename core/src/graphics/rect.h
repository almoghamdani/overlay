#pragma once
#include <cstdint>

namespace overlay {
namespace core {
namespace graphics {

struct Rect {
  size_t height, width;
  size_t x, y;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay