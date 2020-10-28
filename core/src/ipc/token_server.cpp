#include "token_server.h"

#include <algorithm>
#include <loguru/loguru.hpp>
#include <sstream>

#include "authenticate_response.h"
#include "utils/guid.h"
#include "utils/token.h"

namespace overlay {
namespace core {
namespace ipc {

TokenServer::TokenServer() : pipe_(INVALID_HANDLE_VALUE) {}

void TokenServer::StartTokenGeneratorServer(uint16_t rpc_server_port,
                                            std::string server_certificate) {
  std::string pipe_name = utils::token::GeneratePipeName(GetCurrentProcessId());

  // Create named pipe for outbound communication
  pipe_ = CreateNamedPipeA(pipe_name.c_str(), PIPE_ACCESS_OUTBOUND,
                           PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE |
                               PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                           1, sizeof(AuthenticateResponse), 0, 0, NULL);
  CHECK_F(pipe_ != INVALID_HANDLE_VALUE,
          "Unable to create pipe for token generator server! (Error: 0x%x)",
          GetLastError());

  DLOG_F(INFO,
         "Started token generator server with pipe name '%s'. (HANDLE: 0x%x)",
         pipe_name.c_str(), pipe_);

  // Start pipe main thread
  main_thread_ = std::thread(&TokenServer::PipeMainThread, this,
                             rpc_server_port, server_certificate);
}

DWORD TokenServer::GetTokenProcessId(GUID token) {
  std::lock_guard tokens_lk(tokens_mutex_);

  if (!tokens_.count(token)) {
    return 0;
  }

  return tokens_[token];
}

void TokenServer::InvalidateProcessToken(GUID token) {
  std::lock_guard tokens_lk(tokens_mutex_);

  tokens_.erase(token);
}

void TokenServer::PipeMainThread(uint16_t rpc_server_port,
                                 std::string server_certificate) {
  AuthenticateResponse response = {0};
  DWORD written_bytes = 0;

  DWORD process_id;

  std::unique_lock tokens_lk(tokens_mutex_, std::defer_lock);

#ifdef DEBUG
  loguru::set_thread_name("token server");
#endif

  response.rpc_server_port = rpc_server_port;
  std::memcpy(response.server_certificate, server_certificate.data(),
              server_certificate.size() + 1);

  // TODO: Handle stopping threads
  while (true) {
    // If a client is connected
    if (ConnectNamedPipe(pipe_, NULL) ||
        GetLastError() == ERROR_PIPE_CONNECTED) {
      // Get client's process id
      if (GetNamedPipeClientProcessId(pipe_, &process_id)) {
        // TODO: Add verification for processes

        // Generate token for the process
        response.token = GenerateTokenForProcess(process_id);

        // Send the response to the client
        WriteFile(pipe_, &response, sizeof(response), &written_bytes, NULL);
      }
    }

    DisconnectNamedPipe(pipe_);
  }
}

GUID TokenServer::GenerateTokenForProcess(DWORD pid) {
  GUID token;

  std::lock_guard tokens_lk(tokens_mutex_);

  // Check if a token already exists for the process
  for (auto it = tokens_.begin(); it != tokens_.end(); it++) {
    if (it->second == pid) {
      return it->first;
    }
  }

  // Generate random guid
  token = utils::Guid::GenerateGuid();

  // Save token
  tokens_[token] = pid;

  DLOG_F(INFO, "Generated token '%s' for process %d.",
         utils::Guid::GuidToString(&token).c_str(), pid);

  return token;
}

}  // namespace ipc
}  // namespace core
}  // namespace overlay