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
    return (int32_t)point.x >= rect.x &&
           (int32_t)point.x <= (rect.x + (int32_t)rect.width) &&
           (int32_t)point.y >= rect.y &&
           (int32_t)point.y <= (rect.y + (int32_t)rect.height);
  }
};

}  // namespace utils
}  // namespace overlay