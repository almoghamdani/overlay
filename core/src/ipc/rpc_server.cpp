#include "rpc_server.h"

#include <grpcpp/health_check_service_interface.h>

#include <loguru.hpp>

namespace overlay {
namespace core {
namespace ipc {

RpcServer::RpcServer() : server_(), port_(0) {}

void RpcServer::Start() {
  CHECK_F(!server_, "RPC Server is already started!");

  grpc::ServerBuilder server_builder;
  grpc::EnableDefaultHealthCheckService(true);

  // Listen on localhost with a random port and no authentication
  server_builder.AddListeningPort("127.0.0.1:0",
                                  grpc::InsecureServerCredentials(), &port_);

  // Register services
  server_builder.RegisterService(&events_service_);

  // Start the server
  server_ = server_builder.BuildAndStart();

  // Start the server's main thread
  // TODO: Handle stopping threads
  server_thread_ = std::thread([this]() { server_->Wait(); });
  LOG_F(INFO, "RPC Server is running and listening on port %d.", port_);

  token_server_.StartTokenGeneratorServer(port_);
}

}  // namespace ipc
}  // namespace core
}  // namespace overlay