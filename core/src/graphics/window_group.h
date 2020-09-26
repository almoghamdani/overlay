#pragma once
#include <guiddef.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include "window.h"

namespace overlay {
namespace core {
namespace graphics {

struct WindowGroupAttributes {
  int32_t z;
  double opacity;
  bool hidden;
};

struct WindowGroup {
  std::string client_id;

  WindowGroupAttributes attributes;

  std::unordered_map<GUID, std::shared_ptr<Window>> windows;

  std::mutex mutex;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay