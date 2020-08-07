#pragma once
#include <stdint.h>
#include <windows.h>

namespace overlay {

struct AuthenticateResponse {
  uint16_t rpc_server_port;
  GUID token;
};

}  // namespace overlay