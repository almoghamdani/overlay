#include "rpc_server.h"

#include <grpcpp/health_check_service_interface.h>

#include <loguru.hpp>

#include "token_interceptor.h"

namespace overlay {
namespace core {
namespace ipc {

RpcServer::RpcServer() : server_(), port_(0) {}

void RpcServer::Start() {
  CHECK_F(!server_, "RPC Server is already started!");

  std::vector<
      std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>>
      interceptor_creators;

  grpc::ServerBuilder server_builder;
  grpc::EnableDefaultHealthCheckService(true);

  // Listen on localhost with a random port and no authentication
  server_builder.AddListeningPort("localhost:0",
                                  grpc::InsecureServerCredentials(), &port_);

  // Register services
  server_builder.RegisterService(&auth_service_);
  server_builder.RegisterService(&events_service_);

  // Add token interceptor factory to interceptors
  interceptor_creators.push_back(
      std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>(
          new TokenInterceptorFactory()));
  server_builder.experimental().SetInterceptorCreators(
      std::move(interceptor_creators));

  // Start the server
  server_ = server_builder.BuildAndStart();

  // Start the server's main thread
  // TODO: Handle stopping threads
  server_thread_ = std::thread([this]() { server_->Wait(); });
  LOG_F(INFO, "RPC Server is running and listening on port %d.", port_);

  token_server_.StartTokenGeneratorServer(port_);
}

void RpcServer::RegisterClient(std::string client_id, DWORD process_id) {
  LOG_F(INFO, "Registered client with id '%s' to process %d.",
        client_id.c_str(), process_id);

  Client client;
  std::lock_guard client_lk(clients_mutex_);

  client.process_id = process_id;

  clients_[client_id] = client;
}

const Client *RpcServer::GetClient(std::string client_id) {
  std::lock_guard client_lk(clients_mutex_);

  try {
    return &clients_.at(client_id);
  } catch (...) {
    return nullptr;
  }
}

TokenServer *RpcServer::get_token_server() { return &token_server_; }

}  // namespace ipc
}  // namespace core
}  // namespace overlay