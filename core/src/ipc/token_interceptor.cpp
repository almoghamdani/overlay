#include "token_interceptor.h"

#include <grpcpp/server_context.h>
#include <windows.h>

#include <loguru/loguru.hpp>

#include "core.h"

namespace overlay {
namespace core {
namespace ipc {

TokenInterceptor::TokenInterceptor(grpc::experimental::ServerRpcInfo *info)
    : context_(info->server_context()),
      authenticate_rpc_(std::string(info->method())
                            .substr(0, sizeof(AUTHENTICATION_SERVICE) - 1) ==
                        AUTHENTICATION_SERVICE) {}

void TokenInterceptor::Intercept(
    grpc::experimental::InterceptorBatchMethods *methods) {
#ifdef DEBUG
  static thread_local bool set_loguru_thread_name = false;

  if (!set_loguru_thread_name) {
    loguru::set_thread_name("rpc server");
    set_loguru_thread_name = true;
  }
#endif

  // Hook on receiving initial metadata
  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::
              POST_RECV_INITIAL_METADATA) &&
      !authenticate_rpc_) {
    // If the client isn't authenticated
    if (Core::Get()->get_rpc_server()->GetClient(context_->peer()) == nullptr) {
      context_->TryCancel();
      return;
    }
  }

  methods->Proceed();
}

}  // namespace ipc
}  // namespace core
}  // namespace overlay