#include <overlay/error.h>

#include <sstream>

namespace overlay {
namespace helper {

std::string GetErrorCodeDescription(ErrorCode code) {
  switch (code) {
    case ErrorCode::Success:
      return "Success";

    case ErrorCode::CoreDllNotFound:
      return "Unable to find the core DLL of the overlay";

    case ErrorCode::InvalidCoreDll:
      return "The overlay core DLL is invalid";

    case ErrorCode::ProcessNotFound:
      return "The requested process wasn't found";

    default:
    case ErrorCode::UnknownError:
      return "Unknown Error";
  }
}

Error::Error(ErrorCode code) : code_(code) {
  std::stringstream ss;

  ss << "[Overlay Helper Error] " << GetErrorCodeDescription(code)
     << " (Code: " << (int)code << ")";

  error_string_ = ss.str();
}

const char *Error::what() const throw() { return error_string_.c_str(); }

ErrorCode Error::code() const { return code_; }

}  // namespace helper
}  // namespace overlay