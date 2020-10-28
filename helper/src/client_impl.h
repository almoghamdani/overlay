#pragma once
#include <grpcpp/channel.h>
#include <overlay/client.h>
#include <windows.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "authenticate_response.h"
#include "event_manager.h"
#include "utils/guid.h"
#include "window_group_impl.h"
#include "windows.grpc.pb.h"

namespace overlay {
namespace helper {

class ClientImpl : public Client,
                   public std::enable_shared_from_this<ClientImpl> {
 public:
  ClientImpl() = delete;
  ClientImpl(DWORD process_id);

  virtual void Connect();

  virtual void SubscribeToEvent(
      EventType event_type,
      std::function<void(std::shared_ptr<Event>)> callback);
  virtual void UnsubscribeEvent(EventType event_type);

  virtual std::shared_ptr<WindowGroup> CreateWindowGroup(
      const WindowGroupAttributes attributes);

  std::unique_ptr<Windows::Stub> &get_windows_stub();

 private:
  DWORD overlay_pid_;

  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<Windows::Stub> windows_stub_;

  std::unique_ptr<EventManager> event_manager_;

  std::unordered_map<GUID, std::weak_ptr<WindowGroupImpl>> window_groups_;
  std::mutex window_groups_mutex_;

  AuthenticateResponse GetAuthInfo() const;
  std::shared_ptr<grpc::Channel> ConnectToServerChannel() const;

  std::string FormatServerUrl(uint16_t port) const;

  EventResponse::EventCase ConvertEventType(EventType type) const;
  std::shared_ptr<Event> GenerateEvent(EventResponse &response) const;

  void HandleWindowEvent(EventResponse &response);
};

}  // namespace helper
}  // namespace overlay