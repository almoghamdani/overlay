#include <Windows.h>

#include <thread>

#include "graphics/graphics_manager.h"
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

  graphics::GraphicsManager *get_graphics_manager();
  ipc::RpcServer *get_rpc_server();

 private:
  std::thread main_thread_;

  HWND inject_window_;
  HWND graphics_windows_;

  ipc::RpcServer rpc_server_;

  graphics::GraphicsManager graphics_manager_;

  void MainThread();
};

}  // namespace core
}  // namespace overlay