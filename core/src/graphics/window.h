#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "rect.h"

namespace overlay {
namespace core {
namespace graphics {

struct Window {
  std::string client_id;

  Rect rect;
  int32_t z;

  std::vector<uint8_t> buffer;

  std::mutex mutex;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay