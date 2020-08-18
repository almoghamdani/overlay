#ifndef OVERLAY_ERROR_H
#define OVERLAY_ERROR_H
#include <exception>
#include <string>

namespace overlay {
namespace helper {

enum class ErrorCode {
  Success = 0,
  UnknownError,
  CoreDllNotFound,
  InvalidCoreDll,
  ProcessNotFound
};

std::string GetErrorCodeDescription(ErrorCode code);

class Error : public std::exception {
 public:
  Error(ErrorCode code);

  const char* what() const throw();

  ErrorCode code() const;

 private:
  ErrorCode code_;
  std::string error_string_;
};

}  // namespace helper
}  // namespace overlay

#endif