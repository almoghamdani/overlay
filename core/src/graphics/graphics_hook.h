#pragma once
#include <windows.h>

namespace overlay {
namespace core {
namespace graphics {

class IGraphicsHook {
 public:
  inline virtual ~IGraphicsHook() {}

  virtual bool Hook(HWND dummy_window) = 0;
  virtual void Unhook() = 0;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay