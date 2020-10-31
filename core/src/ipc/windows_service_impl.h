#pragma once
#include "windows.grpc.pb.h"

namespace overlay {
namespace core {
namespace ipc {

class WindowsServiceImpl final : public Windows::Service {
 public:
  grpc::Status CreateWindowGroup(grpc::ServerContext *context,
                                 const CreateWindowGroupRequest *request,
                                 CreateWindowGroupResponse *response);
  grpc::Status UpdateWindowGroupProperties(
      grpc::ServerContext *context,
      const UpdateWindowGroupPropertiesRequest *request,
      UpdateWindowGroupPropertiesResponse *response);
  grpc::Status CreateWindowInGroup(grpc::ServerContext *context,
                                   const CreateWindowRequest *request,
                                   CreateWindowResponse *response);
  grpc::Status UpdateWindowProperties(
      grpc::ServerContext *context,
      const UpdateWindowPropertiesRequest *request,
      UpdateWindowPropertiesResponse *response);
  grpc::Status SetWindowRect(grpc::ServerContext *context,
                             const SetWindowRectRequest *request,
                             SetWindowRectResponse *response);
  grpc::Status SetWindowCursor(grpc::ServerContext *context,
                               const SetWindowCursorRequest *request,
                               SetWindowCursorResponse *response);
  grpc::Status BufferForWindow(grpc::ServerContext *context,
                               const BufferForWindowRequest *request,
                               BufferForWindowResponse *response);
};

}  // namespace ipc
}  // namespace core
}  // namespace overlay