#pragma once
#include <unknwn.h>

#include <string>

#include "rect.h"

namespace overlay {
namespace core {
namespace graphics {

struct Sprite {
  Sprite();
  ~Sprite();

  void FreeTexture();

  Rect rect;
  double opacity;

  std::string buffer;
  bool buffer_updated;

  IUnknown *texture;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay