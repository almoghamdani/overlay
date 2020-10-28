#pragma once
#include <guiddef.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include "color.h"
#include "window.h"

namespace overlay {
namespace core {
namespace graphics {

struct WindowGroupAttributes {
  int32_t z;
  double opacity;
  bool hidden;

  bool has_buffer;
  Color buffer_color;
  double buffer_opacity;
};

struct WindowGroup {
  GUID id;
  std::string client_id;

  WindowGroupAttributes attributes;

  std::shared_ptr<Window> buffer_window;

  std::unordered_map<GUID, std::shared_ptr<Window>> windows;
  std::shared_ptr<Window> focused_window;

  std::mutex mutex;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay