#include "events_service_impl.h"

#include <loguru.hpp>

#include "core.h"

namespace overlay {
namespace core {
namespace ipc {

grpc::Status EventsServiceImpl::UnsubscribeEvent(
    grpc::ServerContext *context, const EventUnsubscribeRequest *request,
    EventUnsubscribeResponse *response) {
  if ((EventReply::EventCase)request->type() <=
      EventReply::EventCase::EVENT_NOT_SET) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "Invalid event type");
  }

  std::lock_guard workers_lk(event_workers_mutex_);

  auto its = event_workers_.equal_range((EventReply::EventCase)request->type());

  // Find the worker
  for (auto it = its.first; it != its.second; it++) {
    if (it->second->GetClientId() == context->peer()) {
      it->second->Finish(grpc::Status::OK);
      break;
    }
  }

  return grpc::Status::OK;
}

void EventsServiceImpl::AsyncInitialize(grpc::ServerBuilder &server_builder) {
  completion_queue_ = server_builder.AddCompletionQueue();
}

void EventsServiceImpl::StartHandlingAsyncRpcs() {
  async_rpcs_thread_ = std::thread([this]() {
    void *tag = nullptr;
    bool ok = false;

    // Create the first instance of an event worker for new clients
    new AsyncEventsServiceWorker(this, completion_queue_.get());

    // TODO: Handle stopping threads
    while (true) {
      // Get next tag to handle
      completion_queue_->Next(&tag, &ok);

      if (ok) {
        static_cast<AsyncEventsServiceWorker *>(tag)->Handle();
      }
    }
  });
}

void EventsServiceImpl::RegisterEventWorker(EventReply::EventCase event_type,
                                            AsyncEventsServiceWorker *worker) {
  CHECK_F(event_type > EventReply::EventCase::EVENT_NOT_SET);
  std::lock_guard workers_lk(event_workers_mutex_);

  event_workers_.insert(std::make_pair(event_type, worker));
}

void EventsServiceImpl::RemoveEventWorker(EventReply::EventCase event_type,
                                          AsyncEventsServiceWorker *worker) {
  CHECK_F(event_type > EventReply::EventCase::EVENT_NOT_SET);
  std::lock_guard workers_lk(event_workers_mutex_);

  auto its = event_workers_.equal_range(event_type);

  // Find the worker
  for (auto it = its.first; it != its.second; it++) {
    if (it->second == worker) {
      event_workers_.erase(it);
      break;
    }
  }
}

AsyncEventsServiceWorker::AsyncEventsServiceWorker(
    EventsServiceImpl *service, grpc::ServerCompletionQueue *completion_queue)
    : service_(service),
      completion_queue_(completion_queue),
      writer_(&context_),
      finished_(false),
      registered_(false) {
  service_->RequestSubscribeToEvent(&context_, &request_, &writer_,
                                    completion_queue_, completion_queue_, this);
}

void AsyncEventsServiceWorker::Handle() {
  if (finished_) {
    if (registered_) {
      service_->RemoveEventWorker((EventReply::EventCase)request_.type(), this);
    }

    delete this;
  } else if (!registered_) {
    // Start new worker instance for new clients
    new AsyncEventsServiceWorker(service_, completion_queue_);

    // If the event type is invalid
    if ((EventReply::EventCase)request_.type() <=
        EventReply::EventCase::EVENT_NOT_SET) {
      Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Invalid event type"));
    } else {
      service_->RegisterEventWorker((EventReply::EventCase)request_.type(),
                                    this);
      registered_ = true;
    }
  }
}

void overlay::core::ipc::AsyncEventsServiceWorker::SendEvent(
    overlay::EventReply &event) {
  writer_.Write(event, this);
}

void AsyncEventsServiceWorker::Finish(grpc::Status status) {
  writer_.Finish(status, this);
  finished_ = true;
}

std::string overlay::core::ipc::AsyncEventsServiceWorker::GetClientId() const {
  CHECK_F(registered_ == true && finished_ == false);
  return context_.peer();
}

}  // namespace ipc
}  // namespace core
}  // namespace overlay