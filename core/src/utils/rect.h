#pragma once
#include <windows.h>

#include "graphics/rect.h"

namespace overlay {
namespace utils {

class Rect {
 public:
  template <typename P>
  inline static bool PointInRect(P point, RECT rect) {
    return point.x >= rect.left && point.x <= rect.right &&
           point.y >= rect.top && point.y <= rect.bottom;
  }

  template <typename P>
  inline static bool PointInRect(P point, core::graphics::Rect rect) {
    return (size_t)point.x >= rect.x &&
           (size_t)point.x <= (rect.x + rect.width) &&
           (size_t)point.y >= rect.y &&
           (size_t)point.y <= (rect.y + rect.height);
  }
};

}  // namespace utils
}  // namespace overlay