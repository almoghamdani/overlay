#pragma once
#include <guiddef.h>
#include <overlay/window.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "events.pb.h"

namespace overlay {
namespace helper {

class ClientImpl;
class WindowGroupImpl;

class WindowImpl : public Window {
 public:
  WindowImpl(std::weak_ptr<ClientImpl> client,
             std::shared_ptr<WindowGroupImpl> window_group, GUID id,
             GUID group_id, const Rect rect, const WindowAttributes attributes);

  virtual void SetAttributes(const WindowAttributes attributes);
  virtual const WindowAttributes GetAttributes() const;

  virtual void SetRect(const Rect rect);
  virtual const Rect GetRect() const;

  virtual void UpdateBitmapBuffer(const void* buffer, size_t buffer_size);

  virtual void SubscribeToEvent(
      WindowEventType event_type,
      std::function<void(std::shared_ptr<WindowEvent>)> callback);
  virtual void UnsubscribeEvent(WindowEventType event_type);

  void HandleWindowEvent(const EventResponse::WindowEvent& event);

 private:
  std::weak_ptr<ClientImpl> client_;
  std::shared_ptr<WindowGroupImpl> window_group_;
  GUID id_, group_id_;

  Rect rect_;
  WindowAttributes attributes_;

  std::unordered_map<WindowEventType,
                     std::function<void(std::shared_ptr<WindowEvent>)>>
      event_handlers_;
  std::mutex event_handlers_mutex_;

  std::shared_ptr<WindowEvent> GenerateEvent(
      const EventResponse::WindowEvent& event) const;

  WindowKeyboardInputEvent::KeyCode ConvertVirtualKeyToKeyCode(
      uint8_t virtual_key) const;
};

}  // namespace helper
}  // namespace overlay