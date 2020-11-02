#include "window_manager.h"

#include <algorithm>
#include <loguru/loguru.hpp>

#include "core.h"
#include "utils/guid.h"

namespace overlay {
namespace core {
namespace graphics {

GUID WindowManager::CreateWindowGroup(std::string client_id,
                                      WindowGroupAttributes attributes) {
  std::shared_ptr<WindowGroup> window_group = std::make_shared<WindowGroup>();
  GUID id = utils::Guid::GenerateGuid();

  std::unique_lock window_groups_lk(window_groups_mutex_, std::defer_lock);

  window_group->id = WindowGroupUniqueId(id, client_id);
  window_group->attributes = attributes;
  window_group->focused_window_id = GUID_NULL;

  if (attributes.has_buffer) {
    window_group->buffer_window =
        CreateBufferWindow(attributes.buffer_color, attributes.buffer_opacity);
  }

  window_groups_lk.lock();
  window_groups_[window_group->id] = window_group;
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
    const WindowGroupUniqueId &id, const WindowGroupAttributes &attributes) {
  std::unique_lock sprites_lk(sprites_mutex_, std::defer_lock);
  std::shared_ptr<Sprite> buffer_sprite = nullptr;
  std::vector<std::pair<std::shared_ptr<Sprite>, double>> group_sprites;

  bool update_sprites = false;

  std::shared_ptr<WindowGroup> window_group = nullptr;

  // Get the window group
  try {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_group = window_groups_.at(id);
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

void WindowManager::DestroyWindowGroup(const WindowGroupUniqueId &id) {
  {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_groups_.erase(id);
  }

  // Update the sprites
  UpdateSprites();
}

GUID WindowManager::CreateWindowInGroup(const WindowGroupUniqueId &group_id,
                                        const Rect &rect,
                                        const WindowAttributes &attributes) {
  std::shared_ptr<Window> window = std::make_shared<Window>();
  GUID id = utils::Guid::GenerateGuid();

  std::shared_ptr<WindowGroup> window_group = nullptr;

  // Get the window group
  try {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_group = window_groups_.at(group_id);
  } catch (...) {
    return GUID_NULL;
  }

  window->id = group_id.GenerateWindowId(id);
  window->rect = rect;
  window->attributes = attributes;
  window->cursor = LoadCursor(NULL, IDC_ARROW);
  window->sprite = std::make_shared<Sprite>();
  window->sprite->rect = rect;
  window->sprite->opacity =
      window->attributes.opacity * window_group->attributes.opacity;

  // Add the new window to the list of windows
  {
    std::lock_guard window_group_lk(window_group->mutex);
    window_group->windows[id] = window;

    if (window_group->focused_window_id == GUID_NULL && !attributes.hidden) {
      window_group->focused_window_id = window->id.window_id;
    }
  }

  DLOG_F(INFO,
         "Created new window (ID: '%s', Size: %dx%d) for window group '%s'.",
         utils::Guid::GuidToString(&id).c_str(), rect.width, rect.height,
         utils::Guid::GuidToString(&group_id.group_id).c_str());

  // Update the sprites
  UpdateSprites();

  return id;
}

std::shared_ptr<Window> WindowManager::GetWindowWithId(
    const WindowUniqueId &id) {
  std::shared_ptr<WindowGroup> window_group = nullptr;

  // Get the window group
  try {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_group = window_groups_.at(id.GetGroupId());
  } catch (...) {
    return nullptr;
  }

  try {
    std::lock_guard window_group_lk(window_group->mutex);
    return window_group->windows.at(id.window_id);
  } catch (...) {
    // Window wasn't found
    return nullptr;
  }
}

bool WindowManager::UpdateWindowAttributes(const WindowUniqueId &id,
                                           const WindowAttributes &attributes) {
  std::unique_lock sprites_lk(sprites_mutex_, std::defer_lock);

  std::shared_ptr<Window> window = nullptr;
  std::shared_ptr<WindowGroup> window_group = nullptr;

  std::shared_ptr<Sprite> sprite = nullptr;

  bool update_sprites = false;
  double old_opacity = 0;

  // Get the window group
  try {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_group = window_groups_.at(id.GetGroupId());
  } catch (...) {
    return false;
  }

  try {
    std::lock_guard window_group_lk(window_group->mutex);
    window = window_group->windows.at(id.window_id);

    if (window_group->focused_window_id == id.window_id && attributes.hidden) {
      window_group->focused_window_id = GUID_NULL;
    } else if (window_group->focused_window_id == GUID_NULL &&
               !attributes.hidden) {
      window_group->focused_window_id = id.window_id;
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

bool WindowManager::SetWindowRect(const WindowUniqueId &id, const Rect &rect) {
  std::unique_lock sprites_lk(sprites_mutex_, std::defer_lock);

  std::shared_ptr<Window> window = GetWindowWithId(id);

  std::shared_ptr<Sprite> sprite = nullptr;

  bool regenerate_texture = false;
  double old_opacity = 0;

  if (!window) {
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

bool WindowManager::SetWindowCursor(const WindowUniqueId &id,
                                    const HCURSOR cursor) {
  std::shared_ptr<Window> window = GetWindowWithId(id);

  if (!window) {
    return false;
  }

  std::unique_lock window_lk(window->mutex);
  window->cursor = cursor;
  window_lk.unlock();

  {
    std::unique_lock focused_window_id_lk(focused_window_id_mutex_);

    // If the window is the focused window, then set the
    if (id == focused_window_id_) {
      focused_window_id_lk.unlock();
      Core::Get()->get_input_manager()->set_block_app_input_cursor(cursor);
    }
  }

  return true;
}

void WindowManager::DestroyWindowInGroup(const WindowUniqueId &id) {
  std::shared_ptr<WindowGroup> window_group = nullptr;

  // Get the window group
  try {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_group = window_groups_.at(id.GetGroupId());
  } catch (...) {
    return;
  }

  try {
    std::lock_guard window_group_lk(window_group->mutex);
    window_group->windows.erase(id.window_id);

    if (window_group->focused_window_id == id.window_id) {
      window_group->focused_window_id = GUID_NULL;
    }
  } catch (...) {
  }

  // Update the sprites
  UpdateSprites();
}

void WindowManager::UpdateWindowBufferInGroup(const WindowUniqueId &id,
                                              std::string &&buffer) {
  std::shared_ptr<Window> window = GetWindowWithId(id);
  std::shared_ptr<Sprite> sprite = nullptr;

  if (!window) {
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

const WindowUniqueId WindowManager::GetFocusedWindowId() {
  std::lock_guard focused_window_id_lk(focused_window_id_mutex_);

  return focused_window_id_;
}

void WindowManager::SendWindowEventToFocusedWindow(EventResponse &event) {
  CHECK_F(event.event_case() == EventResponse::kWindowEvent);

  EventResponse::WindowEvent *window_event = event.mutable_windowevent();

  WindowUniqueId focused_window_id = GetFocusedWindowId();
  if (!focused_window_id) {
    return;
  }

  // Set window group id and window id
  window_event->set_windowgroupid((const char *)&focused_window_id.group_id,
                                  sizeof(focused_window_id.group_id));
  window_event->set_windowid((const char *)&focused_window_id.window_id,
                             sizeof(focused_window_id.window_id));

  // Send the event to the client that owns the window
  Core::Get()->get_rpc_server()->get_events_service()->SendEventToClient(
      focused_window_id.client_id, event);
}

std::shared_ptr<Window> WindowManager::GetFocusedWindow() {
  WindowUniqueId focused_window_id = GetFocusedWindowId();

  return focused_window_id ? GetWindowWithId(focused_window_id) : nullptr;
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

  // Add each group's windows
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

    // Move the focused window to the end
    if (window_group->focused_window_id != GUID_NULL) {
      sorted_windows.erase(
          std::remove_if(sorted_windows.end() - window_group->windows.size(),
                         sorted_windows.end(),
                         [id = window_group->focused_window_id](
                             const std::shared_ptr<Window> &window) {
                           return window->id.window_id == id;
                         }));
      sorted_windows.push_back(
          window_group->windows[window_group->focused_window_id]);
    }
  }

  // Remove all hidden windows
  std::copy_if(sorted_windows.begin(), sorted_windows.end(),
               std::back_inserter(sorted_visible_windows),
               [](std::shared_ptr<Window> &window) {
                 std::lock_guard window_lk(window->mutex);
                 return !window->attributes.hidden;
               });

  // Set the focused window
  if (sorted_windows.empty()) {
    FocusWindow(nullptr);
  } else {
    FocusWindow(sorted_windows[sorted_windows.size() - 1]);
  }

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

  const WindowUniqueId focused_window_id = GetFocusedWindowId();
  std::shared_ptr<Window> focused_window =
      focused_window_id ? GetWindowWithId(focused_window_id) : nullptr;
  HCURSOR focused_window_cursor = NULL;

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
  if (focused_window != nullptr) {
    std::unique_lock focused_window_lk(focused_window->mutex);
    focused_window_cursor = focused_window->cursor;
    focused_window_lk.unlock();

    Core::Get()->get_input_manager()->set_block_app_input_cursor(
        focused_window_cursor);
  }
}

void WindowManager::FocusWindow(std::shared_ptr<Window> window) {
  std::unique_lock focused_window_id_lk(focused_window_id_mutex_);
  if (window && focused_window_id_ == window->id) {
    return;
  }

  focused_window_id_ = window && window->id ? window->id : WindowUniqueId();
  focused_window_id_lk.unlock();

  if (!window || !window->id) {
    Core::Get()->get_input_manager()->set_block_app_input_cursor(
        LoadCursor(NULL, IDC_ARROW));
  } else {
    std::lock_guard window_lk(window->mutex);
    Core::Get()->get_input_manager()->set_block_app_input_cursor(
        window->cursor);
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