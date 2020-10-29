#pragma once
#include <guiddef.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "rect.h"
#include "sprite.h"

namespace overlay {
namespace core {
namespace graphics {

struct WindowAttributes {
  double opacity;
  bool hidden;
};

struct Window {
  GUID id, group_id;
  std::string client_id;

  Rect rect;
  WindowAttributes attributes;
  bool focused;

  std::shared_ptr<Sprite> sprite;

  std::mutex mutex;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay