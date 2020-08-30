#pragma once
#include <grpcpp/grpcpp.h>
#include <stdint.h>

#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "auth_service_impl.h"
#include "client.h"
#include "events_service_impl.h"
#include "token_server.h"

namespace overlay {
namespace core {
namespace ipc {

class RpcServer {
 public:
  RpcServer();

  void Start();

  void RegisterClient(std::string client_id, DWORD process_id);
  const Client *GetClient(std::string client_id);
  const std::unordered_map<std::string, Client> GetAllClients();

  TokenServer *get_token_server();
  EventsServiceImpl *get_events_service();

 private:
  std::unique_ptr<grpc::Server> server_;
  int port_;

  grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair_;

  std::unordered_map<std::string, Client> clients_;
  std::mutex clients_mutex_;

  TokenServer token_server_;
  EventsServiceImpl events_service_;
  AuthServiceImpl auth_service_;

  grpc::SslServerCredentialsOptions::PemKeyCertPair GenerateKeyCertPair() const;
};

}  // namespace ipc
}  // namespace core
}  // namespace overlay