#ifndef OVERLAY_CLIENT_H
#define OVERLAY_CLIENT_H
#include <overlay/events.h>
#include <overlay/export.h>
#include <overlay/window.h>
#include <windows.h>

#include <functional>
#include <memory>

namespace overlay {
namespace helper {

class HELPER_EXPORT Client {
 public:
  virtual ~Client();

  virtual void Connect() = 0;

  virtual void SubscribeToEvent(
      EventType event_type,
      std::function<void(std::shared_ptr<Event>)> callback) = 0;
  virtual void UnsubscribeEvent(EventType event_type) = 0;

  virtual std::shared_ptr<WindowGroup> CreateWindowGroup(
      const WindowGroupAttributes &attributes) = 0;
};

HELPER_EXPORT std::shared_ptr<Client> CreateClient(DWORD process_id);

}  // namespace helper
}  // namespace overlay

#endif