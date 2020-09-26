#pragma once
#include <cstdint>

namespace overlay {
namespace core {
namespace graphics {

struct Color {
  Color() : red(0), green(0), blue(0) {}

  Color(uint32_t rgb)
      : red((uint8_t)(rgb >> 16)),
        green((uint8_t)(rgb >> 8)),
        blue((uint8_t)rgb) {}

  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay