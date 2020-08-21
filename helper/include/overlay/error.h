#ifndef OVERLAY_ERROR_H
#define OVERLAY_ERROR_H
#include <overlay/export.h>

#include <exception>
#include <sstream>
#include <string>

namespace overlay {
namespace helper {

enum class ErrorCode {
  Success = 0,
  UnknownError,
  CoreDllNotFound,
  InvalidCoreDll,
  ProcessNotFound,
  AlreadyConnected,
  AuthFailed
};

HELPER_EXPORT std::string GetErrorCodeDescription(ErrorCode code);

class Error : public std::exception {
 public:
  Error(ErrorCode code) : code_(code) {
    std::stringstream ss;

    ss << "[Overlay Helper Error] " << GetErrorCodeDescription(code)
       << " (Code: " << (int)code << ")";

    error_string_ = ss.str();
  }

  const char* what() const throw() { return error_string_.c_str(); }

  ErrorCode code() const { return code_; }

 private:
  ErrorCode code_;
  std::string error_string_;
};

}  // namespace helper
}  // namespace overlay

#endif