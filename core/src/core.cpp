#include "core.h"

#include <MinHook.h>

#include <loguru/loguru.hpp>

namespace overlay {
namespace core {

Core::Core() : instance_(NULL), inject_window_(NULL), graphics_window_(NULL) {}

void Core::Start(HINSTANCE instance) {
  instance_ = instance;

  // Start core main thread
  main_thread_ = std::thread(&Core::MainThread, this);
}

void Core::MainThread() {
#ifdef DEBUG
  loguru::set_thread_name("overlay main");
  LOG_SCOPE_F(INFO, "Initializing overlay");
#endif

  // Init MinHook
  MH_Initialize();

  // Hook graphics
  if (!graphics_manager_.Hook()) {
    DLOG_F(ERROR, "Unable to hook graphics!");
  }

  // Hook input
  if (!input_manager_.Hook()) {
    DLOG_F(ERROR, "Unable to hook input!");
  }

  // Start the RPC server
  rpc_server_.Start();
}

bool Core::HookWindow(HWND window) { return input_manager_.HookWindow(window); }

HINSTANCE Core::get_instance() const { return instance_; }

void Core::set_inject_window(HWND window) {
  DLOG_F(INFO, "Application's main thread window is 0x%x.", window);
  inject_window_ = window;
}

HWND Core::get_inject_window() const { return inject_window_; }

void Core::set_graphics_window(HWND window) {
  DLOG_F(INFO, "Setting graphics window to 0x%x.", window);
  graphics_window_ = window;

  if (!HookWindow(graphics_window_)) {
    DLOG_F(ERROR, "Unable to hook graphics window!", window);
  } else {
    DLOG_F(INFO, "Hooked graphics window successfully!", window);
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
