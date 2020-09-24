#include "window_manager.h"

#include <algorithm>
#include <loguru.hpp>

#include "core.h"
#include "utils/token.h"

namespace overlay {
namespace core {
namespace graphics {

GUID WindowManager::CreateWindowForClient(std::string client_id, Rect rect,
                                          int32_t z) {
  std::shared_ptr<Window> window = std::make_shared<Window>();
  GUID id = utils::Guid::GenerateGuid();

  std::unique_lock windows_lk(windows_mutex_, std::defer_lock);

  window->client_id = client_id;
  window->rect = rect;
  window->z = z;
  window->sprite = nullptr;

  // Add the new window to the list of windows
  windows_lk.lock();
  windows_[id] = window;
  windows_lk.unlock();

  LOG_F(INFO, "Created new window (ID: '%s') for client '%s'.",
        utils::token::TokenToString(&id).c_str(), client_id.c_str());

  return id;
}

void WindowManager::DestroyWindow(GUID id) {
  std::lock_guard windows_lk(windows_mutex_);
  std::shared_ptr<Window> window;

  // If the window exists
  if (windows_.count(id)) {
    window = windows_[id];

    // Lock the window and remove it from the list
    std::lock_guard window_lk(window->mutex);
    windows_.erase(id);
  }
}

std::shared_ptr<Window> WindowManager::GetWindowWithId(GUID id) {
  std::lock_guard lk(windows_mutex_);

  return windows_.count(id) ? windows_[id] : nullptr;
}

void WindowManager::SwapWindowBuffer(GUID id, std::vector<uint8_t> &buffer) {
  std::shared_ptr<Sprite> sprite = std::make_shared<Sprite>();

  std::shared_ptr<Window> window = GetWindowWithId(id);

  if (window == nullptr) {
    return;
  }

  // Swap buffers
  sprite->buffer.swap(buffer);

  std::lock_guard window_lk(window->mutex);
  sprite->rect = window->rect;
  window->sprite.swap(sprite);
}

void WindowManager::RenderWindows(
    std::unique_ptr<IGraphicsRenderer> &renderer) {
  std::unique_lock windows_lk(windows_mutex_);

  std::vector<std::shared_ptr<Sprite>> sprites(windows_.size());
  std::vector<std::shared_ptr<Window>> windows(windows_.size());

  // Convert all windows to sprites
  std::transform(windows_.begin(), windows_.end(), windows.begin(),
                 [](auto &pair) { return pair.second; });
  windows_lk.unlock();

  // Sort windows list
  std::sort(windows.begin(), windows.end(), [](auto &win1, auto &win2) {
    std::unique_lock win1_lk(win1->mutex, std::defer_lock);
    std::unique_lock win2_lk(win2->mutex, std::defer_lock);
    std::lock(win1_lk, win2_lk);

    return win1->z < win2->z;
  });

  // Convert windows list to sprites list
  std::transform(windows.begin(), windows.end(), sprites.begin(),
                 [](auto &window) {
                   std::lock_guard window_lk(window->mutex);
                   return window->sprite;
                 });

  // Render the windows' sprites
  renderer->RenderSprites(sprites);
}

void WindowManager::OnResize() {
  std::lock_guard windows_lk(windows_mutex_);

  // Free all sprites
  for (auto &pair : windows_) {
    std::lock_guard window_lk(pair.second->mutex);

    if (pair.second->sprite) {
      pair.second->sprite->FreeTexture();
    }
  }
}

}  // namespace graphics
}  // namespace core
}  // namespace overlay