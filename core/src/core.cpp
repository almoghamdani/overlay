#include "core.h"

#include <MinHook.h>

#include <loguru.hpp>

namespace overlay {
namespace core {

void Core::Start() {
  // Start core main thread
  main_thread_ = std::thread(&Core::MainThread, this);
}

void Core::MainThread() {
  // Init MinHook
  MH_Initialize();

  // Hook graphics
  if (!graphics_manager_.Hook()) {
    LOG_F(ERROR, "Unable to hook graphics!");
  }

  // Start the RPC server
  rpc_server_.Start();
}

void Core::set_inject_window(HWND window) {
  LOG_F(INFO, "Application's main thread window is 0x%x.", window);
  inject_window_ = window;
}

HWND Core::get_inject_window() const { return inject_window_; }

void Core::set_graphics_window(HWND window) {
  LOG_F(INFO, "Setting graphics window to 0x%x.", window);
  graphics_windows_ = window;
}

HWND Core::get_graphics_window() const { return graphics_windows_; }

graphics::GraphicsManager *Core::get_graphics_manager() {
  return &graphics_manager_;
}

ipc::RpcServer *Core::get_rpc_server() { return &rpc_server_; }

}  // namespace core
}  // namespace overlay
