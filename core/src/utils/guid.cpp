#include "guid.h"

#include <combaseapi.h>

namespace overlay {
namespace utils {

GUID Guid::GenerateGuid() {
  GUID guid;

  CoCreateGuid(&guid);

  return guid;
}

}  // namespace utils
}  // namespace overlay