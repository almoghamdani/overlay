#pragma once
#include <unknwn.h>

#include <string>

#include "color.h"
#include "rect.h"

namespace overlay {
namespace core {
namespace graphics {

struct Sprite {
  Sprite();
  ~Sprite();

  void FreeTexture();

  bool fill_target;
  Rect rect;

  double opacity;

  std::string buffer;
  bool buffer_updated;

  bool solid_color;
  Color color;

  IUnknown *texture;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay