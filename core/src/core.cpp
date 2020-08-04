#include "core.h"

#include <loguru.hpp>

namespace overlay {
namespace core {

void Core::Start() {
  // Start core main thread
  main_thread_ = std::thread(&Core::MainThread, this);
}

void Core::MainThread() {
  // Hook DXGI api (DirectX 10 - 12)
  dxgi_hook_.Hook();
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

graphics::DxgiHook *Core::get_dxgi_hook() { return &dxgi_hook_; }

}  // namespace core
}  // namespace overlay
