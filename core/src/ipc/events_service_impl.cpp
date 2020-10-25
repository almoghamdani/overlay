#include "events_service_impl.h"

#include <loguru.hpp>

#include "core.h"

namespace overlay {
namespace core {
namespace ipc {

grpc::Status EventsServiceImpl::UnsubscribeEvent(
    grpc::ServerContext *context, const EventUnsubscribeRequest *request,
    EventUnsubscribeResponse *response) {
  if ((EventResponse::EventCase)request->type() <=
      EventResponse::EventCase::EVENT_NOT_SET) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "Invalid event type");
  }

  std::lock_guard workers_lk(event_workers_mutex_);

  try {
    event_workers_.at((EventResponse::EventCase)request->type())
        .at(context->peer())
        ->Finish(grpc::Status::OK);
  } catch (...) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND,
                        "No function was subscribed to this event");
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

#ifdef DEBUG
    loguru::set_thread_name("events service");
#endif

    // Create the first instance of an event worker for new clients
    new AsyncEventsServiceWorker(this, completion_queue_.get());

    // TODO: Handle stopping threads
    while (true) {
      // Get next tag to handle
      completion_queue_->Next(&tag, &ok);

      if (ok) {
        static_cast<AsyncEventsServiceWorker *>(tag)->Handle();
      } else if (tag) {
        static_cast<AsyncEventsServiceWorker *>(tag)->ForceFinish();
      }
    }
  });
}

void EventsServiceImpl::RegisterEventWorker(EventResponse::EventCase event_type,
                                            AsyncEventsServiceWorker *worker) {
  CHECK_F(event_type > EventResponse::EventCase::EVENT_NOT_SET);
  std::lock_guard workers_lk(event_workers_mutex_);

  event_workers_[event_type][worker->GetClientId()] = worker;
}

void EventsServiceImpl::RemoveEventWorker(EventResponse::EventCase event_type,
                                          AsyncEventsServiceWorker *worker) {
  CHECK_F(event_type > EventResponse::EventCase::EVENT_NOT_SET);
  std::lock_guard workers_lk(event_workers_mutex_);

  try {
    event_workers_.at(event_type).erase(worker->GetClientId());
  } catch (...) {
  }
}

bool EventsServiceImpl::SendEventToClient(std::string client_id,
                                          EventResponse event) {
  CHECK_F(event.event_case() > EventResponse::EventCase::EVENT_NOT_SET);
  std::lock_guard workers_lk(event_workers_mutex_);

  try {
    event_workers_.at(event.event_case()).at(client_id)->SendEvent(event);
    return true;
  } catch (...) {
    return false;
  }
}

void EventsServiceImpl::BroadcastEvent(EventResponse event) {
  CHECK_F(event.event_case() > EventResponse::EventCase::EVENT_NOT_SET);
  std::lock_guard workers_lk(event_workers_mutex_);

  if (!event_workers_.count(event.event_case())) {
    return;
  }

  auto workers = event_workers_.at(event.event_case());

  for (auto &worker : workers) {
    worker.second->SendEvent(event);
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
    ForceFinish();
  } else if (!registered_) {
    // Start new worker instance for new clients
    new AsyncEventsServiceWorker(service_, completion_queue_);

    // If the event type is invalid
    if ((EventResponse::EventCase)request_.type() <=
        EventResponse::EventCase::EVENT_NOT_SET) {
      Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Invalid event type"));
    } else {
      registered_ = true;
      service_->RegisterEventWorker((EventResponse::EventCase)request_.type(),
                                    this);
    }
  }
}

void overlay::core::ipc::AsyncEventsServiceWorker::SendEvent(
    overlay::EventResponse &event) {
  writer_.Write(event, this);
}

void AsyncEventsServiceWorker::Finish(grpc::Status status) {
  writer_.Finish(status, this);
  finished_ = true;
}

void AsyncEventsServiceWorker::ForceFinish() {
  if (registered_) {
    service_->RemoveEventWorker((EventResponse::EventCase)request_.type(),
                                this);
  }

  delete this;
}

std::string overlay::core::ipc::AsyncEventsServiceWorker::GetClientId() const {
  CHECK_F(registered_ == true);
  return context_.peer();
}

}  // namespace ipc
}  // namespace core
}  // namespace overlay