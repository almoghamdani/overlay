#pragma once
#include <Windows.h>
#include <guiddef.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "color.h"
#include "events.pb.h"
#include "graphics_renderer.h"
#include "sprite.h"
#include "utils/guid.h"
#include "window.h"
#include "window_group.h"

namespace overlay {
namespace core {
namespace graphics {

class WindowManager {
 public:
  GUID CreateWindowGroup(std::string client_id,
                         WindowGroupAttributes attributes);
  bool UpdateWindowGroupAttributes(const WindowGroupUniqueId &id,
                                   const WindowGroupAttributes &attributes);
  void DestroyWindowGroup(const WindowGroupUniqueId &id);

  GUID CreateWindowInGroup(const WindowGroupUniqueId &group_id,
                           const Rect &rect,
                           const WindowAttributes &attributes);
  bool UpdateWindowAttributes(const WindowUniqueId &id,
                              const WindowAttributes &attributes);
  bool SetWindowRect(const WindowUniqueId &id, const Rect &rect);
  bool SetWindowCursor(const WindowUniqueId &id, const HCURSOR cursor);
  bool FocusWindowInGroup(const WindowUniqueId &id);
  void UpdateWindowBufferInGroup(const WindowUniqueId &id,
                                 std::string &&buffer);
  void DestroyWindowInGroup(const WindowUniqueId &id);

  void RenderWindows(std::unique_ptr<IGraphicsRenderer> &renderer);
  void OnResize();

  void SendWindowEventToWindow(EventResponse event,
                               const WindowUniqueId &window_id);
  void SendWindowEventToFocusedWindow(EventResponse event);
  void HandleMouseEvent(EventResponse event, POINT point);

 private:
  std::unordered_map<WindowGroupUniqueId, std::shared_ptr<WindowGroup>>
      window_groups_;
  std::mutex window_groups_mutex_;

  WindowUniqueId focused_window_id_;
  std::mutex focused_window_id_mutex_;

  WindowUniqueId hovered_window_id_;
  std::mutex hovered_window_id_mutex_;

  std::vector<std::pair<WindowUniqueId, Rect>> window_rects_;
  std::mutex window_rects_mutex_;

  std::vector<std::shared_ptr<Sprite>> sprites_;
  std::mutex sprites_mutex_;

  void UpdateWindows();
  void UpdateBlockAppInput();

  void FocusWindow(std::shared_ptr<Window> window);
  void SetHoveredWindow(const WindowUniqueId &window_id);

  std::shared_ptr<Window> CreateBufferWindow(Color color, double opacity);

  std::shared_ptr<Window> GetWindowWithId(const WindowUniqueId &id);

  const WindowUniqueId GetFocusedWindowId();
  const WindowUniqueId GetHoveredWindowId();
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay