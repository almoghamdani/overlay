#pragma once
#include "auth.grpc.pb.h"

namespace overlay {
namespace core {
namespace ipc {

class AuthServiceImpl final : public Authentication::Service {
 public:
  grpc::Status AuthenticateWithToken(grpc::ServerContext *context,
                                     const TokenAuthenticationRequest *request,
                                     TokenAuthenticationResponse *response);
};

}  // namespace ipc
}  // namespace core
}  // namespace overlay