#pragma once
#include <vector>

#include "sprite.h"

namespace overlay {
namespace core {
namespace graphics {

class IGraphicsRenderer {
 public:
  inline virtual ~IGraphicsRenderer() {}

  virtual bool Init() = 0;
  virtual void RenderSprites(const std::vector<Sprite>& sprites) = 0;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay