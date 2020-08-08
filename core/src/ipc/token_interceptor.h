#pragma once

#include <grpcpp/support/server_interceptor.h>

#define AUTHENTICATION_SERVICE "/overlay.Authentication"

namespace overlay {
namespace core {
namespace ipc {

class TokenInterceptor : public grpc::experimental::Interceptor {
 public:
  TokenInterceptor(grpc::experimental::ServerRpcInfo* info);

  void Intercept(grpc::experimental::InterceptorBatchMethods* methods);

 private:
  grpc_impl::ServerContextBase* context_;
  bool authenticate_rpc_;
};

class TokenInterceptorFactory
    : public grpc::experimental::ServerInterceptorFactoryInterface {
 public:
  grpc::experimental::Interceptor* CreateServerInterceptor(
      grpc::experimental::ServerRpcInfo* info) {
    return new TokenInterceptor(info);
  }
};

}  // namespace ipc
}  // namespace core
}  // namespace overlay