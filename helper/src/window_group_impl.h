#pragma once
#include <guiddef.h>
#include <overlay/window.h>

#include <memory>
#include <mutex>
#include <unordered_map>

#include "utils/guid.h"
#include "window_impl.h"

namespace overlay {
namespace helper {

class ClientImpl;

class WindowGroupImpl : public WindowGroup,
                        public std::enable_shared_from_this<WindowGroupImpl> {
 public:
  WindowGroupImpl(std::weak_ptr<ClientImpl> client, GUID id,
                  const WindowGroupAttributes attributes);

  virtual void SetAttributes(const WindowGroupAttributes attributes);
  virtual const WindowGroupAttributes GetAttributes() const;

  virtual std::shared_ptr<Window> CreateNewWindow(
      const WindowAttributes attributes);

  std::shared_ptr<WindowImpl> GetWindowWithId(GUID id);

 private:
  std::weak_ptr<ClientImpl> client_;
  GUID id_;

  WindowGroupAttributes attributes_;

  std::unordered_map<GUID, std::weak_ptr<WindowImpl>> windows_;
  std::mutex windows_mutex_;
};

}  // namespace helper
}  // namespace overlay