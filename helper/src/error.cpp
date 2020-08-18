#include <overlay/error.h>

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

}  // namespace helper
}  // namespace overlay