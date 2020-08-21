#ifndef OVERLAY_CLIENT_H
#define OVERLAY_CLIENT_H
#include <overlay/export.h>
#include <windows.h>

namespace overlay {
namespace helper {

class HELPER_EXPORT Client {
 public:
  Client();
  ~Client();

  void ConnectToOverlay(DWORD pid);

 private:
  struct Impl;
  Impl *pimpl_;
};

}  // namespace helper
}  // namespace overlay

#endif