#include "utils/token.h"

#include <cstdio>
#include <sstream>

namespace overlay {
namespace utils {
namespace token {

std::string GeneratePipeName(DWORD pid) {
  std::stringstream ss;

  ss << "\\\\.\\pipe\\" << PIPE_IDENTIFIER << "-" << pid;

  return ss.str();
}

std::string TokenToString(GUID *token) {
  char guid_string[37];  // 32 hex chars + 4 hyphens + null terminator

  snprintf(guid_string, sizeof(guid_string),
           "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", token->Data1,
           token->Data2, token->Data3, token->Data4[0], token->Data4[1],
           token->Data4[2], token->Data4[3], token->Data4[4], token->Data4[5],
           token->Data4[6], token->Data4[7]);

  return guid_string;
}

}  // namespace token
}  // namespace utils
}  // namespace overlay