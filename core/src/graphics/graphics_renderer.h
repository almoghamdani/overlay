#pragma once
#include <memory>
#include <mutex>
#include <vector>

#include "sprite.h"

namespace overlay {
namespace core {
namespace graphics {

class IGraphicsRenderer {
 public:
  inline virtual ~IGraphicsRenderer() {}

  virtual bool Init() = 0;
  virtual void RenderSprites(
      const std::vector<std::shared_ptr<Sprite>>& sprites) = 0;

  inline void QueueTextureRelease(IUnknown* texture) {
    std::lock_guard lk(texture_release_queue_mutex_);
    texture_release_queue_.push_back(texture);
  }

 protected:
  inline void ReleaseTextures() {
    std::lock_guard lk(texture_release_queue_mutex_);

    // Release the textures
    for (auto& texture : texture_release_queue_) {
      texture->Release();
    }

    // Clear the vector
    texture_release_queue_.clear();
  }

 private:
  std::vector<IUnknown*> texture_release_queue_;
  std::mutex texture_release_queue_mutex_;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay