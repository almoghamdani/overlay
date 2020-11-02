#include "window.h"

#include "window_group.h"

namespace overlay {
namespace core {
namespace graphics {

const WindowGroupUniqueId WindowUniqueId::GetGroupId() const {
  return WindowGroupUniqueId(group_id, client_id);
}

}  // namespace graphics
}  // namespace core
}  // namespace overlay