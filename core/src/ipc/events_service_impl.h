#pragma once
#include "events.grpc.pb.h"

namespace overlay {
namespace core {
namespace ipc {

class EventsServiceImpl final : public Events::Service {
 public:
  grpc::Status SubscribeToEvent(grpc::ServerContext *context,
                                const EventSubscribeRequest *request,
                                grpc::ServerWriter<EventReply> *writer);
};

}  // namespace rpc
}  // namespace core
}  // namespace overlay