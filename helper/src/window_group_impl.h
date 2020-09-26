#pragma once
#include <guiddef.h>
#include <overlay/window.h>

#include <memory>

#include "client_impl.h"

namespace overlay {
namespace helper {

class WindowGroupImpl : public WindowGroup,
                        public std::enable_shared_from_this<WindowGroupImpl> {
 public:
  WindowGroupImpl(std::weak_ptr<ClientImpl> client, GUID id,
                  const WindowGroupAttributes attributes);

  virtual void SetAttributes(const WindowGroupAttributes attributes);
  virtual const WindowGroupAttributes GetAttributes() const;

  virtual std::shared_ptr<Window> CreateNewWindow(
      const WindowAttributes attributes);

 private:
  std::weak_ptr<ClientImpl> client_;
  GUID id_;

  WindowGroupAttributes attributes_;
};

}  // namespace helper
}  // namespace overlay