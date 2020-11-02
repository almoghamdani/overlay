#pragma once
#include <guiddef.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "rect.h"
#include "sprite.h"
#include "utils/guid.h"
#include "utils/hash.h"

namespace overlay {
namespace core {
namespace graphics {
struct WindowGroupUniqueId;

struct WindowUniqueId {
  inline WindowUniqueId() : window_id(GUID_NULL), group_id(GUID_NULL) {}

  inline WindowUniqueId(GUID window_id, GUID group_id, std::string client_id)
      : window_id(window_id), group_id(group_id), client_id(client_id) {}

  const WindowGroupUniqueId GetGroupId() const;

  inline bool Empty() const {
    return window_id == GUID_NULL && group_id == GUID_NULL && client_id.empty();
  }

  inline explicit operator bool() const { return !Empty(); }

  inline bool operator==(const WindowUniqueId& other) const {
    return window_id == other.window_id && group_id == other.group_id &&
           client_id == other.client_id;
  }

  inline bool operator!=(const WindowUniqueId& other) const {
    return !operator==(other);
  }

  GUID window_id, group_id;
  std::string client_id;
};

struct WindowAttributes {
  double opacity;
  bool hidden;
};

struct Window {
  WindowUniqueId id;

  Rect rect;
  WindowAttributes attributes;

  HCURSOR cursor;

  std::shared_ptr<Sprite> sprite;

  std::mutex mutex;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay

namespace std {

template <>
struct hash<overlay::core::graphics::WindowUniqueId> {
  size_t operator()(
      const overlay::core::graphics::WindowUniqueId& value) const {
    size_t seed = 0;
    overlay::utils::Hash::HashCombine(seed, value.window_id);
    overlay::utils::Hash::HashCombine(seed, value.group_id);
    overlay::utils::Hash::HashCombine(seed, value.client_id);
    return seed;
  }
};

}  // namespace std