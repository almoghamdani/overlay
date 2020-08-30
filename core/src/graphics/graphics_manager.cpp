#include "graphics_manager.h"

#include "core.h"
#include "events.pb.h"

namespace overlay {
namespace core {
namespace graphics {

bool GraphicsManager::Hook() { return dxgi_hook_.Hook(); }

DxgiHook *GraphicsManager::get_dxgi_hook() { return &dxgi_hook_; }

StatsCalculator *GraphicsManager::get_stats_calculator() {
  return &stats_calculator_;
}

void GraphicsManager::BroadcastApplicationStats(double frame_time, double fps) {
  EventReply event;
  EventReply::ApplicationStatsEvent *stats_event =
      new EventReply::ApplicationStatsEvent();

  stats_event->set_frametime(frame_time);
  stats_event->set_fps(fps);

  event.set_allocated_applicationstats(stats_event);

  core::Core::Get()->get_rpc_server()->get_events_service()->BroadcastEvent(
      event);
}

}  // namespace graphics
}  // namespace core
}  // namespace overlay