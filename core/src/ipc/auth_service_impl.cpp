#include "auth_service_impl.h"

#include <windows.h>

#include "core.h"

namespace overlay {
namespace core {
namespace ipc {

grpc::Status AuthServiceImpl::AuthenticateWithToken(
    grpc::ServerContext *context, const TokenAuthenticationRequest *request,
    TokenAuthenticationReply *response) {
  GUID token;
  DWORD process_id;

  // If the client is already authenticated
  if (core::Core::Get()->get_rpc_server()->GetClient(context->peer()) !=
      nullptr) {
    return grpc::Status::CANCELLED;
  }

  // If the client is using invalid token, cancel the request
  if (request->token().size() != sizeof(GUID) ||
      (memcpy(&token, request->token().data(), sizeof(GUID)) &&
       (process_id = core::Core::Get()
                         ->get_rpc_server()
                         ->get_token_server()
                         ->GetTokenProcessId(token)) == 0)) {
    return grpc::Status::CANCELLED;
  }

  core::Core::Get()
      ->get_rpc_server()
      ->get_token_server()
      ->InvalidateProcessToken(token);

  core::Core::Get()->get_rpc_server()->RegisterClient(context->peer(),
                                                      process_id);

  return grpc::Status::OK;
}

}  // namespace ipc
}  // namespace core
}  // namespace overlay