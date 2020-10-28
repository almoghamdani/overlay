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

}  // namespace token
}  // namespace utils
}  // namespace overlay