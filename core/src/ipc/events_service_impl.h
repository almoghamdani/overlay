#pragma once
#include <grpcpp/grpcpp.h>

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#pragma warning(push)
#pragma warning(disable : 4127 4244 4267)
#include "events.grpc.pb.h"
#pragma warning(pop)

namespace overlay {
namespace core {
namespace ipc {
class AsyncEventsServiceWorker;

class EventsServiceImpl final
    : public Events::WithAsyncMethod_SubscribeToEvent<Events::Service> {
 public:
  grpc::Status UnsubscribeEvent(grpc::ServerContext *context,
                                const EventUnsubscribeRequest *request,
                                EventUnsubscribeResponse *response);

  void AsyncInitialize(grpc::ServerBuilder &server_builder);
  void StartHandlingAsyncRpcs();

  void RegisterEventWorker(EventResponse::EventCase event_type,
                           AsyncEventsServiceWorker *worker);
  void RemoveEventWorker(EventResponse::EventCase event_type,
                         AsyncEventsServiceWorker *worker);

  bool SendEventToClient(std::string client_id, EventResponse event);
  void BroadcastEvent(EventResponse event);

 private:
  std::unique_ptr<grpc::ServerCompletionQueue> completion_queue_;
  std::thread async_rpcs_thread_;

  std::unordered_map<
      EventResponse::EventCase,
      std::unordered_map<std::string, AsyncEventsServiceWorker *>>
      event_workers_;
  std::mutex event_workers_mutex_;
};

class AsyncEventsServiceWorker {
 public:
  AsyncEventsServiceWorker(EventsServiceImpl *service,
                           grpc::ServerCompletionQueue *completion_queue);

  void Handle();
  void SendEvent(EventResponse &event);

  void Finish(grpc::Status status);
  void ForceFinish();

  std::string GetClientId() const;

 private:
  EventsServiceImpl *service_;

  grpc::ServerCompletionQueue *completion_queue_;
  grpc::ServerContext context_;

  EventSubscribeRequest request_;
  grpc::ServerAsyncWriter<EventResponse> writer_;

  bool writing_;
  std::queue<EventResponse> event_queue_;
  std::mutex event_queue_mutex_;

  std::atomic<bool> finished_, registered_;
};

}  // namespace ipc
}  // namespace core
}  // namespace overlay