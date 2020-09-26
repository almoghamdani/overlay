#include "sprite.h"

#include "core.h"

namespace overlay {
namespace core {
namespace graphics {

Sprite::Sprite()
    : texture(nullptr),
      fill_target(false),
      solid_color(false),
      buffer_updated(false) {}

Sprite::~Sprite() { FreeTexture(); }

void Sprite::FreeTexture() {
  if (texture) {
    std::unique_ptr<IGraphicsRenderer> &renderer =
        Core::Get()->get_graphics_manager()->get_renderer();

    if (renderer) {
      // Queue the texture to be released in the main D3D9 device thread
      renderer->QueueTextureRelease(texture);
    } else {
      texture->Release();
    }

    texture = nullptr;
  }
}

}  // namespace graphics
}  // namespace core
}  // namespace overlay