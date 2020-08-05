#pragma once
#include <rpc.h>
#include <windows.h>

#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#define PIPE_IDENTIFIER "overlay-token-generator"

// Simple hash for GUID
namespace std {

template <>
struct hash<GUID> {
  size_t operator()(const GUID &Value) const {
    RPC_STATUS status = RPC_S_OK;
    return UuidHash(&const_cast<GUID &>(Value), &status);
  }
};

}  // namespace std

namespace overlay {
namespace core {
namespace ipc {

class TokenServer {
 public:
  TokenServer();

  void StartTokenGeneratorServer(uint16_t rpc_server_port);

  DWORD GetTokenProcessId(GUID token);
  void InvalidateProcessToken(GUID token);

 private:
  HANDLE pipe_;
  std::thread main_thread_;

  std::unordered_map<GUID, DWORD> tokens_;
  std::mutex tokens_mutex_;

  void PipeMainThread(uint16_t rpc_server_port);

  std::string GeneratePipeName() const;

  GUID GenerateTokenForProcess(DWORD pid);
  std::string TokenToString(GUID *token);
};

}  // namespace ipc
}  // namespace core
}  // namespace overlay