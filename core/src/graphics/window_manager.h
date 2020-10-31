#pragma once
#include <guiddef.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "color.h"
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
  WindowManager();

  GUID CreateWindowGroup(std::string client_id,
                         WindowGroupAttributes attributes);
  bool UpdateWindowGroupAttributes(GUID id, WindowGroupAttributes attributes);
  std::string GetWindowGroupClientId(GUID id);
  void DestroyWindowGroup(GUID id);

  GUID CreateWindowInGroup(GUID group_id, Rect rect,
                           WindowAttributes attributes);
  bool UpdateWindowAttributes(GUID group_id, GUID window_id,
                              WindowAttributes attributes);
  bool SetWindowRect(GUID group_id, GUID window_id, Rect rect);
  bool SetWindowCursor(GUID group_id, GUID window_id, HCURSOR cursor);
  void UpdateWindowBufferInGroup(GUID group_id, GUID window_id,
                                 std::string&& buffer);
  void DestroyWindowInGroup(GUID group_id, GUID window_id);

  void RenderWindows(std::unique_ptr<IGraphicsRenderer>& renderer);
  void OnResize();

  std::shared_ptr<Window> GetFocusedWindow();

 private:
  std::unordered_map<GUID, std::shared_ptr<WindowGroup>> window_groups_;
  std::shared_ptr<WindowGroup> focused_window_group_;
  std::mutex window_groups_mutex_;

  std::vector<std::shared_ptr<Sprite>> sprites_;
  std::mutex sprites_mutex_;

  void UpdateSprites();
  void UpdateBlockAppInput();

  void FocusWindow(std::shared_ptr<Window> window);

  std::shared_ptr<Window> CreateBufferWindow(Color color, double opacity);
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay