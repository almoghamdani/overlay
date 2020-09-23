#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "rect.h"
#include "sprite.h"

namespace overlay {
namespace core {
namespace graphics {

struct Window {
  std::string client_id;

  Rect rect;
  int32_t z;

  std::shared_ptr<Sprite> sprite;

  std::mutex mutex;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay