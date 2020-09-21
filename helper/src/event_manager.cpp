#include "event_manager.h"

namespace overlay {
namespace helper {

EventManager::EventManager(std::shared_ptr<grpc::Channel> &channel)
    : events_stub_(Events::NewStub(channel)), stop_async_rpcs_(false) {}

EventManager::~EventManager() {
  stop_async_rpcs_ = true;
  completion_queue_.Shutdown();
  async_rpcs_thread_.join();
}

void EventManager::StartHandlingAsyncRpcs() {
  async_rpcs_thread_ = std::thread([this]() {
    void *tag = nullptr;
    bool ok = false;

    while (true) {
      // Get next tag to handle
      if (!completion_queue_.Next(&tag, &ok)) {
        break;
      }

      if (ok && !stop_async_rpcs_) {
        static_cast<AsyncEventListenerWorker *>(tag)->Handle();
      } else if (tag) {
        delete tag;
      }
    }
  });
}

void EventManager::HandleEvent(EventResponse &response) {
  std::unique_lock handlers_lk(event_handlers_mutex_);

  std::function<void(EventResponse &)> handler = nullptr;

  if (event_handlers_.count(response.event_case())) {
    handler = event_handlers_[response.event_case()];
  } else {
    return;
  }

  handlers_lk.unlock();
  handler(response);
}

void EventManager::SubscribeToEvent(
    EventResponse::EventCase event_type,
    std::function<void(EventResponse &)> handler) {
  EventSubscribeRequest request;

  std::lock_guard handlers_lk(event_handlers_mutex_);

  request.set_type(event_type);

  // If there isn't a reader for the event, create one
  if (!event_handlers_.count(event_type)) {
    new AsyncEventListenerWorker(this, events_stub_.get(), &completion_queue_,
                                 request);
  }

  event_handlers_[event_type] = handler;
}

void EventManager::UnsubscribeEvent(EventResponse::EventCase event_type) {
  grpc::ClientContext context;
  EventUnsubscribeRequest request;
  EventUnsubscribeResponse response;

  std::lock_guard handlers_lk(event_handlers_mutex_);

  request.set_type(event_type);

  if (event_handlers_.count(event_type)) {
    events_stub_->UnsubscribeEvent(&context, request, &response);
    event_handlers_.erase(event_type);
  }
}

AsyncEventListenerWorker::AsyncEventListenerWorker(
    EventManager *event_manager, Events::Stub *stub,
    grpc::CompletionQueue *completion_queue, EventSubscribeRequest &request)
    : event_manager_(event_manager),
      reader_(stub->PrepareAsyncSubscribeToEvent(&context_, request,
                                                 completion_queue)),
      started_(false) {
  reader_->StartCall(this);
}

void overlay::helper::AsyncEventListenerWorker::Handle() {
  if (!started_) {
    started_ = true;
  } else {
    event_manager_->HandleEvent(response_);
  }

  // Read the next response
  reader_->Read(&response_, this);
}

}  // namespace helper
}  // namespace overlay