#include <Windows.h>

#include <thread>

#include "graphics/dxgi_hook.h"
#include "ipc/rpc_server.h"
#include "utils/singleton.h"

namespace overlay {
namespace core {

class Core : public utils::Singleton<Core> {
 public:
  void Start();

  void set_inject_window(HWND window);
  HWND get_inject_window() const;

  void set_graphics_window(HWND window);
  HWND get_graphics_window() const;

  graphics::DxgiHook *get_dxgi_hook();
  ipc::RpcServer *get_rpc_server();

 private:
  std::thread main_thread_;

  HWND inject_window_;
  HWND graphics_windows_;

  ipc::RpcServer rpc_server_;

  graphics::DxgiHook dxgi_hook_;

  void MainThread();
};

}  // namespace core
}  // namespace overlay