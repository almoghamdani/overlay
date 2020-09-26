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

  virtual void OnResize(size_t width, size_t height, bool fullscreen) = 0;

  inline void QueueTextureRelease(IUnknown* texture) {
    std::lock_guard lk(texture_release_queue_mutex_);
    texture_release_queue_.push_back(texture);
  }

  inline size_t get_width() const { return width_; }
  inline size_t get_height() const { return height_; }
  inline bool is_fullscreen() const { return fullscreen_; }

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

  inline void set_width(size_t width) { width_ = width; }
  inline void set_height(size_t height) { height_ = height; }
  inline void set_fullscreen(bool fullscreen) { fullscreen_ = fullscreen; }

 private:
  size_t width_, height_;
  bool fullscreen_;

  std::vector<IUnknown*> texture_release_queue_;
  std::mutex texture_release_queue_mutex_;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay