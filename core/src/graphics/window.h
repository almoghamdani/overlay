#pragma once
#include <cstdint>
#include <memory>
#include <mutex>

#include "rect.h"
#include "sprite.h"

namespace overlay {
namespace core {
namespace graphics {

struct WindowAttributes {
  Rect rect;
  int32_t z;
  double opacity;
  bool hidden;
};

struct Window {
  WindowAttributes attributes;

  std::shared_ptr<Sprite> sprite;

  std::mutex mutex;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay