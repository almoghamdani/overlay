#pragma once
#include <cstdint>
#include <vector>

namespace overlay {
namespace core {
namespace graphics {

struct Sprite {
  std::vector<uint8_t> buffer;

  uint64_t height, width;
  uint64_t x, y;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay