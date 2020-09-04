#pragma once
#include "dx9_hook.h"
#include "dxgi_hook.h"
#include "stats_calculator.h"

namespace overlay {
namespace core {
namespace graphics {

class GraphicsManager {
 public:
  bool Hook();

  Dx9Hook *get_dx9_hook();
  DxgiHook *get_dxgi_hook();

  StatsCalculator *get_stats_calculator();

  void BroadcastApplicationStats(double frame_time, double fps);

 private:
  Dx9Hook dx9_hook_;
  DxgiHook dxgi_hook_;

  StatsCalculator stats_calculator_;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay