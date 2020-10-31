#ifndef OVERLAY_WINDOW_H
#define OVERLAY_WINDOW_H
#include <overlay/color.h>
#include <overlay/cursor.h>
#include <overlay/export.h>
#include <overlay/rect.h>
#include <overlay/window_events.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace overlay {
namespace helper {

struct WindowGroupAttributes {
  int32_t z;
  double opacity;
  bool hidden;

  bool has_buffer;
  Color buffer_color;
  double buffer_opacity;
};

struct WindowAttributes {
  double opacity;
  bool hidden;
};

class HELPER_EXPORT Window {
 public:
  virtual ~Window();

  virtual void SetAttributes(const WindowAttributes attributes) = 0;
  virtual const WindowAttributes GetAttributes() const = 0;

  virtual void SetRect(const Rect rect) = 0;
  virtual const Rect GetRect() const = 0;

  virtual void SetCursor(const Cursor cursor) = 0;
  virtual const Cursor GetCursor() const = 0;

  virtual void UpdateBitmapBuffer(const void* buffer, size_t buffer_size) = 0;

  inline void UpdateBitmapBuffer(std::string& buffer) {
    return UpdateBitmapBuffer(buffer.data(), buffer.size());
  }

  template <typename T>
  inline void UpdateBitmapBuffer(std::vector<T>& buffer) {
    return UpdateBitmapBuffer(buffer.data(), buffer.size() * sizeof(T));
  }

  virtual void SubscribeToEvent(
      WindowEventType event_type,
      std::function<void(std::shared_ptr<WindowEvent>)> callback) = 0;
  virtual void UnsubscribeEvent(WindowEventType event_type) = 0;
};

class HELPER_EXPORT WindowGroup {
 public:
  virtual ~WindowGroup();

  virtual void SetAttributes(const WindowGroupAttributes attributes) = 0;
  virtual const WindowGroupAttributes GetAttributes() const = 0;

  virtual std::shared_ptr<Window> CreateNewWindow(
      const Rect rect, const WindowAttributes attributes) = 0;
};

}  // namespace helper
}  // namespace overlay

#endif