#include "core.h"

#include <MinHook.h>

#include <loguru.hpp>

namespace overlay {
namespace core {

Core::Core()
    : inject_window_(NULL), graphics_window_(NULL) {}

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

  // Hook input
  if (!input_manager_.Hook()) {
    LOG_F(ERROR, "Unable to hook input!");
  }

  // Start the RPC server
  rpc_server_.Start();
}

bool Core::HookWindow(HWND window) {
  return input_manager_.HookWindow(window);
}

void Core::set_inject_window(HWND window) {
  LOG_F(INFO, "Application's main thread window is 0x%x.", window);
  inject_window_ = window;
}

HWND Core::get_inject_window() const { return inject_window_; }

void Core::set_graphics_window(HWND window) {
  LOG_F(INFO, "Setting graphics window to 0x%x.", window);
  graphics_window_ = window;

  if(!HookWindow(graphics_window_)) {
    LOG_F(ERROR, "Unable to hook graphics window!", window);
  } else {
    LOG_F(INFO, "Hooked graphics window successfully!", window);
  }
}

HWND Core::get_graphics_window() const { return graphics_window_; }

graphics::GraphicsManager *Core::get_graphics_manager() {
  return &graphics_manager_;
}

input::InputManager *Core::get_input_manager() { return &input_manager_; }

ipc::RpcServer *Core::get_rpc_server() { return &rpc_server_; }

}  // namespace core
}  // namespace overlay
