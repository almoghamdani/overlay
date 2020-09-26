#ifndef OVERLAY_COLOR_H
#define OVERLAY_COLOR_H
#include <cstdint>

namespace overlay {
namespace helper {

struct Color {
  Color() : red(0), green(0), blue(0) {}

  Color(uint8_t red, uint8_t green, uint8_t blue)
      : red(red), green(green), blue(blue) {}

  Color(uint32_t rgb)
      : red((uint8_t)(rgb >> 16)),
        green((uint8_t)(rgb >> 8)),
        blue((uint8_t)rgb) {}

  constexpr operator uint32_t() const {
    return ((uint32_t)red << 16) + ((uint32_t)green << 8) + blue;
  }

  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

}  // namespace helper
}  // namespace overlay

#endif