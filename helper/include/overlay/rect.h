#ifndef OVERLAY_RECT_H
#define OVERLAY_RECT_H
#include <stddef.h>

namespace overlay {
namespace helper {

struct Rect {
  size_t height, width;
  size_t x, y;
};

}  // namespace helper
}  // namespace overlay

#endif