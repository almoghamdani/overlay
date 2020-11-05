#include "window_manager.h"

#include <algorithm>
#include <loguru/loguru.hpp>

#include "core.h"
#include "utils/guid.h"
#include "utils/rect.h"

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
    UpdateWindows();
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
    UpdateWindows();
  }

  return true;
}

void WindowManager::DestroyWindowGroup(const WindowGroupUniqueId &id) {
  {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_groups_.erase(id);
  }

  // Update the windows
  UpdateWindows();
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

  // Update the windows
  UpdateWindows();

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
    UpdateWindows();
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

  // If the window is the hovered window, then set the cursor
  if (id == GetHoveredWindowId()) {
    Core::Get()->get_input_manager()->set_block_app_input_cursor(cursor);
  }

  return true;
}

bool WindowManager::FocusWindowInGroup(const WindowUniqueId &id) {
  std::shared_ptr<Window> window = nullptr;
  std::shared_ptr<WindowGroup> window_group = nullptr;

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
  } catch (...) {
    // Window wasn't found
    return false;
  }

  {
    // Verify that the window isn't hidden
    std::lock_guard window_lk(window->mutex);
    if (window->attributes.hidden) {
      return false;
    }
  }

  {
    std::lock_guard window_group_lk(window_group->mutex);
    window_group->focused_window_id = id.window_id;
  }

  UpdateWindows();

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

  // Update the windows
  UpdateWindows();
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

const WindowUniqueId WindowManager::GetHoveredWindowId() {
  std::lock_guard hovered_window_id_lk(hovered_window_id_mutex_);
  return hovered_window_id_;
}

void WindowManager::SendWindowEventToWindow(EventResponse event,
                                            const WindowUniqueId &window_id) {
  CHECK_F(event.event_case() == EventResponse::kWindowEvent && window_id);

  // Ignore invalid windows
  if (!GetWindowWithId(window_id)) {
    return;
  }

  EventResponse::WindowEvent *window_event = event.mutable_windowevent();

  // Set window group id and window id
  window_event->set_windowgroupid((const char *)&window_id.group_id,
                                  sizeof(window_id.group_id));
  window_event->set_windowid((const char *)&window_id.window_id,
                             sizeof(window_id.window_id));

  // Send the event to the client that owns the window
  Core::Get()->get_rpc_server()->get_events_service()->SendEventToClient(
      window_id.client_id, event);
}

void WindowManager::SendWindowEventToFocusedWindow(EventResponse event) {
  CHECK_F(event.event_case() == EventResponse::kWindowEvent);

  WindowUniqueId focused_window_id = GetFocusedWindowId();
  if (!focused_window_id) {
    return;
  }

  SendWindowEventToWindow(event, focused_window_id);
}

void WindowManager::HandleMouseEvent(EventResponse event, POINT point) {
  CHECK_F(event.event_case() == EventResponse::kWindowEvent);

  WindowUniqueId focused_window_id = GetFocusedWindowId();
  WindowUniqueId new_hovered_window_id;

  std::vector<std::pair<WindowUniqueId, Rect>> window_rects;
  {
    std::lock_guard window_rects_lk(window_rects_mutex_);
    window_rects = window_rects_;
  }

  EventResponse::WindowEvent *window_event = event.mutable_windowevent();
  EventResponse::WindowEvent::MouseInputEvent *input_event =
      window_event->mutable_mouseinputevent();

  bool in_rect = false, focused = false;

  for (auto window_it = window_rects.rbegin(); window_it != window_rects.rend();
       window_it++) {
    in_rect = utils::Rect::PointInRect(point, window_it->second);
    focused = window_it->first == focused_window_id;

    // If the event is in the window or the window is the focused window
    if (in_rect || focused) {
      // Send mouse move and mouse button up to focused window even if not in
      // rect or send every event except mouse button up to the window the event
      // occurred in
      if ((focused && !in_rect &&
           !(input_event->type() ==
                 EventResponse::WindowEvent::MouseInputEvent::MOUSE_MOVE ||
             input_event->type() == EventResponse::WindowEvent::
                                        MouseInputEvent::MOUSE_BUTTON_UP)) ||
          (in_rect && !focused &&
           input_event->type() ==
               EventResponse::WindowEvent::MouseInputEvent::MOUSE_BUTTON_UP)) {
        continue;
      }

      input_event->set_x(point.x - window_it->second.x);
      input_event->set_y(point.y - window_it->second.y);

      SendWindowEventToWindow(event, window_it->first);

      // Focus on the window that was pressed
      if (input_event->type() ==
          EventResponse::WindowEvent::MouseInputEvent::MOUSE_BUTTON_DOWN) {
        FocusWindowInGroup(window_it->first);
      } else if (input_event->type() ==
                     EventResponse::WindowEvent::MouseInputEvent::MOUSE_MOVE &&
                 in_rect) {
        new_hovered_window_id = window_it->first;
      }

      // If we found the window the event occurred in, break
      if (in_rect) {
        break;
      }
    }
  }

  // If the mouse moved, update the hovered window
  if (input_event->type() ==
      EventResponse::WindowEvent::MouseInputEvent::MOUSE_MOVE) {
    SetHoveredWindow(new_hovered_window_id);
  }
}

void WindowManager::SetHoveredWindow(const WindowUniqueId &window_id) {
  std::shared_ptr<Window> window = GetWindowWithId(window_id);
  WindowUniqueId new_hovered_window_id =
      window ? window_id
             : WindowUniqueId();  // Treat invalid windows as no window

  std::unique_lock hovered_window_id_lk(hovered_window_id_mutex_);
  if (new_hovered_window_id == hovered_window_id_) {
    return;
  }

  hovered_window_id_ = new_hovered_window_id;
  hovered_window_id_lk.unlock();

  if (!window) {
    Core::Get()->get_input_manager()->set_block_app_input_cursor(
        LoadCursor(NULL, IDC_ARROW));
  } else {
    std::lock_guard window_lk(window->mutex);
    Core::Get()->get_input_manager()->set_block_app_input_cursor(
        window->cursor);
  }
}

void WindowManager::UpdateWindows() {
  std::vector<std::pair<WindowUniqueId, Rect>> window_rects;
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

  // Transform windows into sprites and rects
  for (const auto &window : sorted_windows) {
    std::lock_guard window_lk(window->mutex);

    sprites.push_back(window->sprite);
    window_rects.push_back(std::make_pair(window->id, window->rect));
  }

  {
    // Swap the sprites vector
    std::lock_guard sprites_lk(sprites_mutex_);
    sprites_.swap(sprites);
  }

  {
    // Swap the rects vector
    std::lock_guard window_rects_lk(window_rects_mutex_);
    window_rects_.swap(window_rects);
  }
}

void WindowManager::UpdateBlockAppInput() {
  bool block_input = false;

  const WindowUniqueId hovered_window_id = GetHoveredWindowId();
  std::shared_ptr<Window> hovered_window =
      hovered_window_id ? GetWindowWithId(hovered_window_id) : nullptr;
  HCURSOR hovered_window_cursor = NULL;

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

  // Set hovered window's cursor
  if (hovered_window != nullptr) {
    std::unique_lock hovered_window_lk(hovered_window->mutex);
    hovered_window_cursor = hovered_window->cursor;
    hovered_window_lk.unlock();

    Core::Get()->get_input_manager()->set_block_app_input_cursor(
        hovered_window_cursor);
  }
}

void WindowManager::FocusWindow(std::shared_ptr<Window> window) {
  std::lock_guard focused_window_id_lk(focused_window_id_mutex_);
  if (window && focused_window_id_ == window->id) {
    return;
  }

  focused_window_id_ = window && window->id ? window->id : WindowUniqueId();
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