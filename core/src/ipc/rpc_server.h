#pragma once
#include <grpcpp/grpcpp.h>
#include <stdint.h>

#include <memory>
#include <thread>

#include "events_service_impl.h"
#include "token_server.h"

namespace overlay {
namespace core {
namespace ipc {

class RpcServer {
 public:
  RpcServer();

  void Start();

 private:
  std::unique_ptr<grpc::Server> server_;
  int port_;

  std::thread server_thread_;

  TokenServer token_server_;
  EventsServiceImpl events_service_;
};

}  // namespace ipc
}  // namespace core
}  // namespace overlay