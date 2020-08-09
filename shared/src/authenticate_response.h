#pragma once
#include <stdint.h>
#include <windows.h>

#define SERVER_CERTIFICATE_MAX_SIZE 2000

namespace overlay {

struct AuthenticateResponse {
  uint16_t rpc_server_port;
  GUID token;
  char server_certificate[SERVER_CERTIFICATE_MAX_SIZE];
};

}  // namespace overlay