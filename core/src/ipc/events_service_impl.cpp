#include "events_service_impl.h"

namespace overlay {
namespace core {
namespace ipc {

grpc::Status EventsServiceImpl::SubscribeToEvent(
    grpc::ServerContext *context, const EventSubscribeRequest *request,
    grpc::ServerWriter<EventReply> *writer) {
		return grpc::Status::OK;
	}

}  // namespace rpc
}  // namespace core
}  // namespace overlay