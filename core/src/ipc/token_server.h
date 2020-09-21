#pragma once
#include <windows.h>

#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "utils/guid.h"

namespace overlay {
namespace core {
namespace ipc {

class TokenServer {
 public:
  TokenServer();

  void StartTokenGeneratorServer(uint16_t rpc_server_port,
                                 std::string server_certificate);

  DWORD GetTokenProcessId(GUID token);
  void InvalidateProcessToken(GUID token);

 private:
  HANDLE pipe_;
  std::thread main_thread_;

  std::unordered_map<GUID, DWORD> tokens_;
  std::mutex tokens_mutex_;

  void PipeMainThread(uint16_t rpc_server_port, std::string server_certificate);

  GUID GenerateTokenForProcess(DWORD pid);
};

}  // namespace ipc
}  // namespace core
}  // namespace overlay