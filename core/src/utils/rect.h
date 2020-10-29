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
    return point.x >= rect.x && point.x <= (rect.x + rect.width) &&
           point.y >= rect.y && point.y <= (rect.y + rect.height);
  }
};

}  // namespace utils
}  // namespace overlay