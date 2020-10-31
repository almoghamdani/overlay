#include "window_manager.h"

#include <algorithm>
#include <loguru/loguru.hpp>

#include "core.h"
#include "utils/guid.h"

namespace overlay {
namespace core {
namespace graphics {

WindowManager::WindowManager() : focused_window_group_(nullptr) {}

GUID WindowManager::CreateWindowGroup(std::string client_id,
                                      WindowGroupAttributes attributes) {
  std::shared_ptr<WindowGroup> window_group = std::make_shared<WindowGroup>();
  GUID id = utils::Guid::GenerateGuid();

  std::unique_lock window_groups_lk(window_groups_mutex_, std::defer_lock);

  window_group->id = id;
  window_group->client_id = client_id;
  window_group->attributes = attributes;
  window_group->focused_window = nullptr;

  if (attributes.has_buffer) {
    window_group->buffer_window =
        CreateBufferWindow(attributes.buffer_color, attributes.buffer_opacity);
  }

  window_groups_lk.lock();
  window_groups_[id] = window_group;

  if (focused_window_group_ == nullptr && !attributes.hidden) {
    focused_window_group_ = window_group;
  }

  window_groups_lk.unlock();

  UpdateBlockAppInput();

  if (attributes.has_buffer) {
    UpdateSprites();
  }

  DLOG_F(INFO, "Created new window group (ID: '%s') for client '%s'.",
         utils::Guid::GuidToString(&id).c_str(), client_id.c_str());

  return id;
}

bool WindowManager::UpdateWindowGroupAttributes(
    GUID id, WindowGroupAttributes attributes) {
  std::unique_lock sprites_lk(sprites_mutex_, std::defer_lock);
  std::shared_ptr<Sprite> buffer_sprite = nullptr;
  std::vector<std::pair<std::shared_ptr<Sprite>, double>> group_sprites;

  bool update_sprites = false;

  std::shared_ptr<WindowGroup> window_group = nullptr;

  // Get the window group
  try {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_group = window_groups_.at(id);

    if (focused_window_group_ == window_group && attributes.hidden) {
      focused_window_group_ = nullptr;
    } else if (focused_window_group_ == nullptr && !attributes.hidden) {
      focused_window_group_ = window_group;
    }
  } catch (...) {
    return false;
  }

  std::unique_lock window_group_lk(window_group->mutex);

  // Check if there is a need of a sprites update
  if (window_group->attributes.z != attributes.z ||
      window_group->attributes.hidden != attributes.hidden ||
      window_group->attributes.has_buffer != attributes.has_buffer) {
    update_sprites = true;
  }

  // Save sprites for re-calculation of opacity
  if (window_group->attributes.opacity != attributes.opacity) {
    for (auto &window_pair : window_group->windows) {
      std::lock_guard window_lk(window_pair.second->mutex);
      group_sprites.push_back(std::make_pair(
          window_pair.second->sprite,
          window_pair.second->attributes.opacity * attributes.opacity));
    }
  }

  // Create buffer window if needed
  if (attributes.has_buffer) {
    if (window_group->buffer_window == nullptr) {
      window_group->buffer_window = CreateBufferWindow(
          attributes.buffer_color, attributes.buffer_opacity);
    } else {
      window_group->buffer_window->attributes.opacity =
          attributes.buffer_opacity;
      buffer_sprite = window_group->buffer_window->sprite;
    }
  }

  // Update attributes
  window_group->attributes = attributes;
  window_group_lk.unlock();

  // Update sprites' opacity
  if (!group_sprites.empty()) {
    sprites_lk.lock();
    for (auto &sprite_pair : group_sprites) {
      sprite_pair.first->opacity = sprite_pair.second;
    }
    sprites_lk.unlock();
  }

  // Update buffer sprite color and opacity
  if (buffer_sprite != nullptr) {
    sprites_lk.lock();
    buffer_sprite->opacity = attributes.buffer_opacity;
    buffer_sprite->color = attributes.buffer_color;
    sprites_lk.unlock();
  }

  UpdateBlockAppInput();

  if (update_sprites) {
    UpdateSprites();
  }

  return true;
}

std::string WindowManager::GetWindowGroupClientId(GUID id) {
  std::lock_guard window_groups_lk(window_groups_mutex_);

  try {
    return window_groups_[id]->client_id;
  } catch (...) {
    return nullptr;
  }
}

void WindowManager::DestroyWindowGroup(GUID id) {
  {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_groups_.erase(id);
  }

  // Update the sprites
  UpdateSprites();
}

GUID WindowManager::CreateWindowInGroup(GUID group_id, Rect rect,
                                        WindowAttributes attributes) {
  // * The window group must exist

  std::shared_ptr<Window> window = std::make_shared<Window>();
  GUID id = utils::Guid::GenerateGuid();

  std::shared_ptr<WindowGroup> window_group = nullptr;

  // Get the window group
  {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_group = window_groups_[group_id];
  }

  window->id = id;
  window->group_id = group_id;
  window->client_id = window_group->client_id;
  window->rect = rect;
  window->attributes = attributes;
  window->focused = false;
  window->cursor = LoadCursor(NULL, IDC_ARROW);
  window->sprite = std::make_shared<Sprite>();
  window->sprite->rect = rect;
  window->sprite->opacity =
      window->attributes.opacity * window_group->attributes.opacity;

  // Add the new window to the list of windows
  {
    std::lock_guard window_group_lk(window_group->mutex);
    window_group->windows[id] = window;

    if (window_group->focused_window == nullptr && !attributes.hidden) {
      window_group->focused_window = window;
      window->focused = true;
    }
  }

  DLOG_F(INFO,
         "Created new window (ID: '%s', Size: %dx%d) for window group '%s'.",
         utils::Guid::GuidToString(&id).c_str(), rect.width, rect.height,
         utils::Guid::GuidToString(&group_id).c_str());

  // Update the sprites
  UpdateSprites();

  return id;
}

bool WindowManager::UpdateWindowAttributes(GUID group_id, GUID window_id,
                                           WindowAttributes attributes) {
  std::unique_lock sprites_lk(sprites_mutex_, std::defer_lock);

  std::shared_ptr<Window> window = nullptr;
  std::shared_ptr<WindowGroup> window_group = nullptr;

  std::shared_ptr<Sprite> sprite = nullptr;

  bool update_sprites = false;
  double old_opacity = 0;

  // Get the window group
  try {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_group = window_groups_.at(group_id);
  } catch (...) {
    return false;
  }

  try {
    std::lock_guard window_group_lk(window_group->mutex);
    window = window_group->windows.at(window_id);

    if (window_group->focused_window == window && attributes.hidden) {
      window_group->focused_window = nullptr;
    } else if (window_group->focused_window == nullptr && !attributes.hidden) {
      window_group->focused_window = window;
    }
  } catch (...) {
    // Window wasn't found
    return false;
  }

  std::unique_lock window_lk(window->mutex);

  // Check if sprites needs to be updated
  if (window->attributes.hidden != attributes.hidden) {
    update_sprites = true;
  }

  sprite = window->sprite;
  old_opacity = window->attributes.opacity;
  window->attributes = attributes;
  window_lk.unlock();

  sprites_lk.lock();
  sprite->opacity = (sprite->opacity / old_opacity) * attributes.opacity;
  sprites_lk.unlock();

  if (update_sprites) {
    UpdateSprites();
  }

  return true;
}

bool WindowManager::SetWindowRect(GUID group_id, GUID window_id, Rect rect) {
  std::unique_lock sprites_lk(sprites_mutex_, std::defer_lock);

  std::shared_ptr<Window> window = nullptr;
  std::shared_ptr<WindowGroup> window_group = nullptr;

  std::shared_ptr<Sprite> sprite = nullptr;

  bool regenerate_texture = false;
  double old_opacity = 0;

  // Get the window group
  try {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_group = window_groups_.at(group_id);
  } catch (...) {
    return false;
  }

  try {
    std::lock_guard window_group_lk(window_group->mutex);
    window = window_group->windows.at(window_id);
  } catch (...) {
    // Window wasn't found
    return false;
  }

  std::unique_lock window_lk(window->mutex);

  // Check if the texture needs to be regenerated
  if (window->rect.height != rect.height || window->rect.width != rect.width) {
    regenerate_texture = true;
  }

  sprite = window->sprite;
  window->rect = rect;
  window_lk.unlock();

  sprites_lk.lock();

  if (regenerate_texture) {
    sprite->FreeTexture();
  }

  sprite->rect = rect;

  return true;
}

bool WindowManager::SetWindowCursor(GUID group_id, GUID window_id,
                                    HCURSOR cursor) {
  std::shared_ptr<Window> window = nullptr;
  std::shared_ptr<WindowGroup> window_group = nullptr;

  // Get the window group
  try {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_group = window_groups_.at(group_id);
  } catch (...) {
    return false;
  }

  try {
    std::lock_guard window_group_lk(window_group->mutex);
    window = window_group->windows.at(window_id);
  } catch (...) {
    // Window wasn't found
    return false;
  }

  std::unique_lock window_lk(window->mutex);
  window->cursor = cursor;
  window_lk.unlock();

  // If the window is the focused window, then set the
  if (window == GetFocusedWindow()) {
    Core::Get()->get_input_manager()->set_block_app_input_cursor(cursor);
  }

  return true;
}

void WindowManager::DestroyWindowInGroup(GUID group_id, GUID window_id) {
  std::shared_ptr<WindowGroup> window_group = nullptr;

  // Get the window group
  try {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_group = window_groups_.at(group_id);
  } catch (...) {
    return;
  }

  try {
    std::lock_guard window_group_lk(window_group->mutex);
    window_group->windows.erase(window_id);
  } catch (...) {
  }
}

void WindowManager::UpdateWindowBufferInGroup(GUID group_id, GUID window_id,
                                              std::string &&buffer) {
  std::shared_ptr<Window> window = nullptr;
  std::shared_ptr<WindowGroup> window_group = nullptr;

  std::shared_ptr<Sprite> sprite = nullptr;

  // Get the window group
  try {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_group = window_groups_.at(group_id);
  } catch (...) {
    return;
  }
  try {
    std::lock_guard window_group_lk(window_group->mutex);
    window = window_group->windows.at(window_id);
  } catch (...) {
    // Window wasn't found
    return;
  }

  std::unique_lock window_lk(window->mutex);
  sprite = window->sprite;
  window_lk.unlock();

  std::lock_guard sprites_lk(sprites_mutex_);
  sprite->buffer = std::move(buffer);
  sprite->buffer_updated = true;
}

void WindowManager::RenderWindows(
    std::unique_ptr<IGraphicsRenderer> &renderer) {
  // Render the windows' sprites
  std::lock_guard sprites_lk(sprites_mutex_);
  renderer->RenderSprites(sprites_);
}

void WindowManager::OnResize() {
  std::unique_ptr<IGraphicsRenderer> &renderer =
      Core::Get()->get_graphics_manager()->get_renderer();

  std::lock_guard sprites_lk(sprites_mutex_);

  // Release all textures
  for (auto &sprite : sprites_) {
    sprite->FreeTexture();
  }
}

std::shared_ptr<Window> WindowManager::GetFocusedWindow() {
  std::lock_guard window_groups_lk(window_groups_mutex_);

  if (focused_window_group_ != nullptr) {
    std::lock_guard window_group_lk(focused_window_group_->mutex);
    return focused_window_group_->focused_window;
  }

  return nullptr;
}

void WindowManager::UpdateSprites() {
  std::vector<std::shared_ptr<Sprite>> sprites;

  std::vector<std::shared_ptr<WindowGroup>> window_groups;
  std::vector<std::shared_ptr<WindowGroup>> sorted_window_groups;

  std::vector<std::shared_ptr<Window>> sorted_windows;
  std::vector<std::shared_ptr<Window>> sorted_visible_windows;

  // Get all window groups
  {
    std::lock_guard window_groups_lk(window_groups_mutex_);

    std::transform(window_groups_.begin(), window_groups_.end(),
                   std::back_inserter(window_groups),
                   [](auto &pair) { return pair.second; });
  }

  // Remove all hidden window groups
  std::copy_if(window_groups.begin(), window_groups.end(),
               std::back_inserter(sorted_window_groups),
               [](std::shared_ptr<WindowGroup> &group) {
                 std::lock_guard group_lk(group->mutex);
                 return !group->attributes.hidden;
               });

  // Sort all window groups
  std::sort(sorted_window_groups.begin(), sorted_window_groups.end(),
            [](std::shared_ptr<WindowGroup> &group1,
               std::shared_ptr<WindowGroup> &group2) {
              std::unique_lock group1_lk(group1->mutex, std::defer_lock);
              std::unique_lock group2_lk(group2->mutex, std::defer_lock);
              std::lock(group1_lk, group2_lk);

              return group1->attributes.z < group2->attributes.z;
            });

  // Sort windows for each group
  for (auto &window_group : sorted_window_groups) {
    std::lock_guard group_lk(window_group->mutex);

    // Create buffer sprite
    if (window_group->attributes.has_buffer &&
        window_group->buffer_window != nullptr) {
      sorted_windows.push_back(window_group->buffer_window);
    }

    // Get all windows
    std::transform(window_group->windows.begin(), window_group->windows.end(),
                   std::back_inserter(sorted_windows),
                   [](auto &pair) { return pair.second; });

    // Sort windows list
    std::sort(sorted_windows.end() - window_group->windows.size(),
              sorted_windows.end(),
              [](std::shared_ptr<Window> &win1, std::shared_ptr<Window> &win2) {
                std::unique_lock win1_lk(win1->mutex, std::defer_lock);
                std::unique_lock win2_lk(win2->mutex, std::defer_lock);
                std::lock(win1_lk, win2_lk);

                return win1->focused < win2->focused;
              });
  }

  // Remove all hidden windows
  std::copy_if(sorted_windows.begin(), sorted_windows.end(),
               std::back_inserter(sorted_visible_windows),
               [](std::shared_ptr<Window> &window) {
                 std::lock_guard window_lk(window->mutex);
                 return !window->attributes.hidden;
               });

  // Transform windows into sprites
  std::transform(sorted_visible_windows.begin(), sorted_visible_windows.end(),
                 std::back_inserter(sprites),
                 [](std::shared_ptr<Window> &window) {
                   std::lock_guard window_lk(window->mutex);
                   return window->sprite;
                 });

  // Swap the sprites vector
  std::lock_guard sprites_lk(sprites_mutex_);
  sprites_.swap(sprites);
}

void WindowManager::UpdateBlockAppInput() {
  bool block_input = false;

  std::shared_ptr<Window> focused_window = GetFocusedWindow();
  HCURSOR focused_window_cursor_name = NULL;

  std::unique_lock window_groups_lk(window_groups_mutex_);

  for (auto &group_pair : window_groups_) {
    std::lock_guard window_group_lk(group_pair.second->mutex);

    if (group_pair.second->attributes.has_buffer &&
        !group_pair.second->attributes.hidden) {
      block_input = true;
      break;
    }
  }

  window_groups_lk.unlock();
  Core::Get()->get_input_manager()->set_block_app_input(block_input);

  // Set focused window's cursor
  if (focused_window) {
    std::unique_lock focused_window_lk(focused_window->mutex);
    focused_window_cursor_name = focused_window->cursor;
    focused_window_lk.unlock();

    Core::Get()->get_input_manager()->set_block_app_input_cursor(
        focused_window_cursor_name);
  }
}

std::shared_ptr<Window> WindowManager::CreateBufferWindow(Color color,
                                                          double opacity) {
  std::shared_ptr<Window> window = std::make_shared<Window>();

  window->attributes.hidden = false;
  window->attributes.opacity = opacity;
  window->sprite = std::make_shared<Sprite>();

  window->sprite->fill_target = true;
  window->sprite->opacity = opacity;
  window->sprite->solid_color = true;
  window->sprite->color = color;

  return window;
}

}  // namespace graphics
}  // namespace core
}  // namespace overlay