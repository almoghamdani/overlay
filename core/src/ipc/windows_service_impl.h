#pragma once
#include "windows.grpc.pb.h"

namespace overlay {
namespace core {
namespace ipc {

class WindowsServiceImpl final : public Windows::Service {
 public:
  grpc::Status CreateNewWindow(grpc::ServerContext *context,
                               const CreateWindowRequest *request,
                               CreateWindowResponse *response);
  grpc::Status BufferForWindow(grpc::ServerContext *context,
                               const BufferForWindowRequest *request,
                               BufferForWindowResponse *response);
};

}  // namespace ipc
}  // namespace core
}  // namespace overlay