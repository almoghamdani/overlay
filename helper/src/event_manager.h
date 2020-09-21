#pragma once

#include <grpcpp/channel.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "events.grpc.pb.h"

namespace overlay {
namespace helper {

class EventManager {
 public:
  EventManager(std::shared_ptr<grpc::Channel> &channel);
  ~EventManager();

  void StartHandlingAsyncRpcs();

  void HandleEvent(EventResponse &response);

  void SubscribeToEvent(EventResponse::EventCase event_type,
                        std::function<void(EventResponse &)> handler);
  void UnsubscribeEvent(EventResponse::EventCase event_type);

 private:
  std::unique_ptr<Events::Stub> events_stub_;

  std::atomic<bool> stop_async_rpcs_;
  grpc::CompletionQueue completion_queue_;
  std::thread async_rpcs_thread_;

  std::unordered_map<EventResponse::EventCase,
                     std::function<void(EventResponse &)>>
      event_handlers_;
  std::mutex event_handlers_mutex_;
};

class AsyncEventListenerWorker {
 public:
  AsyncEventListenerWorker(EventManager *event_manager, Events::Stub *stub,
                           grpc::CompletionQueue *completion_queue,
                           EventSubscribeRequest &request);

  void Handle();

 private:
  EventManager *event_manager_;

  grpc::ClientContext context_;
  std::unique_ptr<grpc::ClientAsyncReader<EventResponse>> reader_;

  EventResponse response_;

  bool started_;
};

}  // namespace helper
}  // namespace overlay