#ifndef OVERLAY_WINDOW_H
#define OVERLAY_WINDOW_H
#include <overlay/export.h>

#include <memory>
#include <string>
#include <vector>

namespace overlay {
namespace helper {

struct WindowGroupAttributes {
  int32_t z;
  double opacity;
  bool hidden;
};

struct WindowAttributes {
  size_t height, width;
  size_t x, y;
  int32_t z;
  double opacity;
  bool hidden;
};

class HELPER_EXPORT Window {
 public:
  virtual ~Window();

  virtual void SetAttributes(const WindowAttributes attributes) = 0;
  virtual const WindowAttributes GetAttributes() const = 0;

  virtual void UpdateBitmapBuffer(const void* buffer, size_t buffer_size) = 0;

  inline void UpdateBitmapBuffer(std::string& buffer) {
    return UpdateBitmapBuffer(buffer.data(), buffer.size());
  }

  template <typename T>
  inline void UpdateBitmapBuffer(std::vector<T>& buffer) {
    return UpdateBitmapBuffer(buffer.data(), buffer.size() * sizeof(T));
  }
};

class HELPER_EXPORT WindowGroup {
 public:
  virtual ~WindowGroup();

  virtual void SetAttributes(const WindowGroupAttributes attributes) = 0;
  virtual const WindowGroupAttributes GetAttributes() const = 0;

  virtual std::shared_ptr<Window> CreateNewWindow(
      const WindowAttributes attributes) = 0;
};

}  // namespace helper
}  // namespace overlay

#endif