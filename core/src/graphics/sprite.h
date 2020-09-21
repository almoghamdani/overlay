#pragma once
#include <cstdint>
#include <vector>

#include "rect.h"

namespace overlay {
namespace core {
namespace graphics {

struct Sprite {
  Rect rect;
  std::vector<uint8_t> buffer;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay