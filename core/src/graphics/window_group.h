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

struct WindowGroupUniqueId {
  inline WindowGroupUniqueId() : group_id(GUID_NULL) {}

  inline WindowGroupUniqueId(GUID group_id, std::string client_id)
      : group_id(group_id), client_id(client_id) {}

  inline const WindowUniqueId GenerateWindowId(GUID id) const {
    return WindowUniqueId(id, group_id, client_id);
  }

  inline bool Empty() const {
    return group_id == GUID_NULL && client_id.empty();
  }

  inline explicit operator bool() const {
    return !Empty();
  }

  inline bool operator==(const WindowGroupUniqueId& other) const {
    return group_id == other.group_id && client_id == other.client_id;
  }

  inline bool operator!=(const WindowGroupUniqueId& other) const {
    return !operator==(other);
  }

  GUID group_id;
  std::string client_id;
};

struct WindowGroupAttributes {
  int32_t z;
  double opacity;
  bool hidden;

  bool has_buffer;
  Color buffer_color;
  double buffer_opacity;
};

struct WindowGroup {
  WindowGroupUniqueId id;

  WindowGroupAttributes attributes;

  std::shared_ptr<Window> buffer_window;

  std::unordered_map<GUID, std::shared_ptr<Window>> windows;
  GUID focused_window_id;

  std::mutex mutex;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay

namespace std {

template <>
struct hash<overlay::core::graphics::WindowGroupUniqueId> {
  size_t operator()(
      const overlay::core::graphics::WindowGroupUniqueId& value) const {
    size_t seed = 0;
    overlay::utils::Hash::HashCombine(seed, value.group_id);
    overlay::utils::Hash::HashCombine(seed, value.client_id);
    return seed;
  }
};

}  // namespace std