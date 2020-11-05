#ifndef OVERLAY_RECT_H
#define OVERLAY_RECT_H
#include <cstdint>

namespace overlay {
namespace helper {

struct Rect {
  uint32_t height, width;
  int32_t x, y;
};

}  // namespace helper
}  // namespace overlay

#endif