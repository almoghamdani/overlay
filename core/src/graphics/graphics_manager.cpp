#include "graphics_manager.h"

#include <windows.h>

#include <loguru.hpp>

#include "core.h"
#include "events.pb.h"

namespace overlay {
namespace core {
namespace graphics {

GraphicsManager::GraphicsManager() : renderer_(nullptr) {}

bool GraphicsManager::Hook() {
  bool hooked = false;

  // Create a dummy window to try and hook to
  HWND dummy_window = CreateWindowA("STATIC", "OVERLAY_DUMMY_WINDOW", WS_POPUP,
                                    0, 0, 2, 2, HWND_MESSAGE, NULL, NULL, NULL);
  if (!dummy_window) {
    return false;
  }

  if (dx9_hook_.Hook(dummy_window)) {
    LOG_F(INFO, "Hooked DirectX 9 successfully!");
    hooked = true;
  } else {
    dx9_hook_.Unhook();
  }

  if (dxgi_hook_.Hook(dummy_window)) {
    LOG_F(INFO, "Hooked DXGI successfully!");
    hooked = true;
  } else {
    dxgi_hook_.Unhook();
  }

  // Destroy the dummy window
  DestroyWindow(dummy_window);

  return hooked;
}

void GraphicsManager::BroadcastApplicationStats(double frame_time, double fps) {
  EventResponse event;
  EventResponse::ApplicationStatsEvent *stats_event =
      new EventResponse::ApplicationStatsEvent();

  stats_event->set_frametime(frame_time);
  stats_event->set_fps(fps);

  event.set_allocated_applicationstats(stats_event);

  Core::Get()->get_rpc_server()->get_events_service()->BroadcastEvent(event);
}

void GraphicsManager::Render() {
  if (renderer_) {
    window_mananger_.RenderWindows(renderer_);
  }
}

WindowManager *GraphicsManager::get_window_manager() {
  return &window_mananger_;
}

Dx9Hook *GraphicsManager::get_dx9_hook() { return &dx9_hook_; }

DxgiHook *GraphicsManager::get_dxgi_hook() { return &dxgi_hook_; }

StatsCalculator *GraphicsManager::get_stats_calculator() {
  return &stats_calculator_;
}

void GraphicsManager::set_renderer(
    std::unique_ptr<IGraphicsRenderer> &&renderer) {
  renderer_ = std::move(renderer);
}

std::unique_ptr<IGraphicsRenderer> &GraphicsManager::get_renderer() {
  return renderer_;
}

}  // namespace graphics
}  // namespace core
}  // namespace overlay