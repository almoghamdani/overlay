#pragma once
#include <grpcpp/grpcpp.h>
#include <stdint.h>

#include <memory>
#include <thread>

#include "events_service_impl.h"

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

  EventsServiceImpl events_service_;
};

}  // namespace rpc
}  // namespace core
}  // namespace overlay