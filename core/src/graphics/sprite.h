#pragma once
#include <cstdint>
#include <vector>

namespace overlay {
namespace core {
namespace graphics {

struct Sprite {
  std::vector<uint8_t> buffer;

  size_t height, width;
  size_t x, y;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay