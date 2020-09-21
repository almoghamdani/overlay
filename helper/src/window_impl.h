#pragma once
#include <guiddef.h>
#include <overlay/window.h>

#include <memory>

#include "client_impl.h"

namespace overlay {
namespace helper {

class WindowImpl : public Window {
 public:
  WindowImpl(std::weak_ptr<ClientImpl> client, GUID id,
             const WindowProperties& properties);

  virtual void UpdateBitmapBuffer(std::vector<uint8_t>& buffer);

 private:
  std::weak_ptr<ClientImpl> client_;
  GUID id_;

  WindowProperties properties_;
};

}  // namespace helper
}  // namespace overlay