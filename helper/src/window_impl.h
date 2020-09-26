#pragma once
#include <guiddef.h>
#include <overlay/window.h>

#include <memory>

#include "client_impl.h"

namespace overlay {
namespace helper {

class WindowGroupImpl;

class WindowImpl : public Window {
 public:
  WindowImpl(std::weak_ptr<ClientImpl> client,
             std::shared_ptr<WindowGroupImpl> window_group, GUID id,
             GUID group_id, const WindowAttributes& attributes);

  virtual const WindowAttributes GetAttributes() const;

  virtual void UpdateBitmapBuffer(const void* buffer, size_t buffer_size);

 private:
  std::weak_ptr<ClientImpl> client_;
  std::shared_ptr<WindowGroupImpl> window_group_;
  GUID id_, group_id_;

  WindowAttributes attributes_;
};

}  // namespace helper
}  // namespace overlay