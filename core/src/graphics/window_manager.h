#pragma once
#include <guiddef.h>

#include <memory>
#include <mutex>
#include <unordered_map>

#include "graphics_renderer.h"
#include "utils/guid.h"
#include "window.h"

namespace overlay {
namespace core {
namespace graphics {

class WindowManager {
 public:
  GUID CreateWindowForClient(std::string client_id, Rect rect, int32_t z);
  void DestroyWindow(GUID id);
  std::shared_ptr<Window> GetWindowWithId(GUID id);
  void SwapWindowBuffer(GUID id, std::vector<uint8_t>& buffer);

  void RenderWindows(std::unique_ptr<IGraphicsRenderer>& renderer);

 private:
  std::unordered_map<GUID, std::shared_ptr<Window>> windows_;
  std::mutex windows_mutex_;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay