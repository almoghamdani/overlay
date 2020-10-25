#include "rpc_server.h"

#include <grpcpp/health_check_service_interface.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <loguru.hpp>

#include "token_interceptor.h"

namespace overlay {
namespace core {
namespace ipc {

RpcServer::RpcServer() : server_(), port_(0) {}

void RpcServer::Start() {
  CHECK_F(!server_, "RPC Server has already started!");

  std::vector<
      std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>>
      interceptor_creators;

  grpc::SslServerCredentialsOptions ssl_options;

  grpc::ServerBuilder server_builder;
  grpc::EnableDefaultHealthCheckService(true);

  // Create private key and certificate for server
  key_cert_pair_ = GenerateKeyCertPair();

  // Set the private key and certificate for the SSL server
  ssl_options.pem_key_cert_pairs.push_back(key_cert_pair_);

  // Listen on localhost with a random port and SSL authentication
  server_builder.AddListeningPort(
      "localhost:0", grpc::SslServerCredentials(ssl_options), &port_);

  // Register servicesS
  server_builder.RegisterService(&auth_service_);
  server_builder.RegisterService(&events_service_);
  server_builder.RegisterService(&windows_service_);

  // Add token interceptor factory to interceptors
  interceptor_creators.push_back(
      std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>(
          new TokenInterceptorFactory()));
  server_builder.experimental().SetInterceptorCreators(
      std::move(interceptor_creators));

  // Initialize async services
  events_service_.AsyncInitialize(server_builder);

  // Start the server
  server_ = server_builder.BuildAndStart();
  events_service_.StartHandlingAsyncRpcs();
  DLOG_F(INFO, "RPC Server is running and listening on port %d.", port_);

  token_server_.StartTokenGeneratorServer(port_, key_cert_pair_.cert_chain);
}

void RpcServer::RegisterClient(std::string client_id, DWORD process_id) {
  DLOG_F(INFO, "Registered client with id '%s' to process %d.",
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

const std::unordered_map<std::string, Client> RpcServer::GetAllClients() {
  std::lock_guard client_lk(clients_mutex_);

  return clients_;
}

grpc::SslServerCredentialsOptions::PemKeyCertPair
RpcServer::GenerateKeyCertPair() const {
  grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair;

  EVP_PKEY *pkey = nullptr;
  RSA *rsa = nullptr;
  X509 *x509 = nullptr;
  X509_NAME *name = nullptr;

  BIO *key_bio = nullptr, *cert_bio = nullptr;
  BUF_MEM *buf = nullptr;

  // Create private key object
  pkey = EVP_PKEY_new();
  CHECK_F(pkey != nullptr, "Unable to create private key object!");

  // Generate RSA key
  rsa = RSA_generate_key(4096, RSA_F4, NULL, NULL);
  CHECK_F(rsa != nullptr, "Unable to generate RSA key!");

  // Assign the RSA key to the private key
  EVP_PKEY_assign_RSA(pkey, rsa);

  // Create new x509 certificate object
  x509 = X509_new();
  CHECK_F(x509 != nullptr, "Unable to create x509 certificate object!");

  // Set the certificate to be valid for 365 days
  X509_gmtime_adj(X509_get_notBefore(x509), 0);
  X509_gmtime_adj(X509_get_notAfter(x509), 60 * 60 * 24 * 365);

  // Set public key for certificate
  X509_set_pubkey(x509, pkey);

  // Set properties
  name = X509_get_subject_name(x509);
  X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"US", -1,
                             -1, 0);
  X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,
                             (unsigned char *)"Overlay", -1, -1, 0);
  X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                             (unsigned char *)"localhost", -1, -1, 0);
  X509_set_issuer_name(x509, name);

  // Sign the certificate
  X509_sign(x509, pkey, EVP_sha1());

  // Convert key to string
  key_bio = BIO_new(BIO_s_mem());
  CHECK_F(
      PEM_write_bio_PrivateKey(key_bio, pkey, NULL, NULL, 0, NULL, NULL) != 0,
      "Unable to write private key to buffer");
  BIO_get_mem_ptr(key_bio, &buf);
  key_cert_pair.private_key = std::string(buf->data, buf->length);

  // Convert cert to string
  cert_bio = BIO_new(BIO_s_mem());
  CHECK_F(PEM_write_bio_X509(cert_bio, x509) != 0,
          "Unable to write x509 certificate to buffer");
  BIO_get_mem_ptr(cert_bio, &buf);
  key_cert_pair.cert_chain = std::string(buf->data, buf->length);

  // Free objects
  EVP_PKEY_free(pkey);
  X509_free(x509);
  BIO_free(key_bio);
  BIO_free(cert_bio);

  return key_cert_pair;
}

TokenServer *RpcServer::get_token_server() { return &token_server_; }

EventsServiceImpl *RpcServer::get_events_service() { return &events_service_; }

}  // namespace ipc
}  // namespace core
}  // namespace overlay