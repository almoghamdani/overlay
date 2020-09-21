#include "window_manager.h"

#include <algorithm>
#include <loguru.hpp>

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
  std::shared_ptr<Window> window = GetWindowWithId(id);
  if (!window) {
    return;
  }

  std::lock_guard window_lk(window->mutex);
  window->buffer.swap(buffer);
}

void WindowManager::RenderWindows(
    std::unique_ptr<IGraphicsRenderer> &renderer) {
  std::unique_lock windows_lk(windows_mutex_);

  std::vector<Sprite> sprites(windows_.size());
  std::vector<std::shared_ptr<Window>> windows(windows_.size());

  // Convert all windows to sprites
  std::transform(windows_.begin(), windows_.end(), windows.begin(),
                 [](auto &pair) { return pair.second; });
  windows_lk.unlock();

  // Sort windows list
  std::sort(windows.begin(), windows.end(),
            [](auto &win1, auto &win2) { return win1->z < win2->z; });

  // Convert windows list to sprites list
  std::transform(windows.begin(), windows.end(), sprites.begin(),
                 [](auto &window) {
                   Sprite sprite;
                   std::lock_guard window_lk(window->mutex);

                   sprite.rect = window->rect;
                   sprite.buffer = window->buffer;

                   return sprite;
                 });

  // Render the windows' sprites
  renderer->RenderSprites(sprites);
}

}  // namespace graphics
}  // namespace core
}  // namespace overlay