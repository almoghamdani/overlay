#pragma once
#include "dxgi_hook.h"
#include "stats_calculator.h"

namespace overlay {
namespace core {
namespace graphics {

class GraphicsManager {
 public:
  bool Hook();

  DxgiHook *get_dxgi_hook();
  StatsCalculator *get_stats_calculator();

  void BroadcastApplicationStats(double frame_time, double fps);

 private:
  DxgiHook dxgi_hook_;

  StatsCalculator stats_calculator_;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay