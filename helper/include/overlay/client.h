#ifndef OVERLAY_CLIENT_H
#define OVERLAY_CLIENT_H
#include <overlay/events.h>
#include <overlay/export.h>
#include <windows.h>

#include <functional>
#include <memory>

namespace overlay {
namespace helper {

class HELPER_EXPORT Client {
 public:
  Client();
  ~Client();

  void ConnectToOverlay(DWORD pid);

  void SubscribeToEvent(EventType event_type,
                        std::function<void(std::shared_ptr<Event>)> callback);
  void UnsubscribeEvent(EventType event_type);

 private:
  struct Impl;
  Impl *pimpl_;
};

}  // namespace helper
}  // namespace overlay

#endif