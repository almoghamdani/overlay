#pragma once
#include <unknwn.h>

#include <cstdint>
#include <vector>

#include "rect.h"

namespace overlay {
namespace core {
namespace graphics {

struct Sprite {
  Sprite();
  ~Sprite();

  Rect rect;

  std::vector<uint8_t> buffer;
  IUnknown *texture;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay