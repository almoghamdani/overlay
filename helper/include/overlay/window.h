#ifndef OVERLAY_WINDOW_H
#define OVERLAY_WINDOW_H
#include <overlay/export.h>

#include <cstdint>
#include <vector>

namespace overlay {
namespace helper {

struct WindowProperties {
  size_t height, width;
  size_t x, y;
  int32_t z;
};

class HELPER_EXPORT Window {
 public:
  virtual ~Window();

  virtual void UpdateBitmapBuffer(std::vector<uint8_t> &buffer) = 0;
};

}  // namespace helper
}  // namespace overlay

#endif