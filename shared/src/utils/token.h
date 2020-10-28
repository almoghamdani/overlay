#pragma once
#include <windows.h>

#include <string>

#define PIPE_IDENTIFIER "overlay-token-generator"

namespace overlay {
namespace utils {
namespace token {

std::string GeneratePipeName(DWORD pid);

}  // namespace token
}  // namespace utils
}  // namespace overlay