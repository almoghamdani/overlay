#pragma once
#include <memory>

#include "dx9_hook.h"
#include "dxgi_hook.h"
#include "graphics_renderer.h"
#include "stats_calculator.h"
#include "window_manager.h"

namespace overlay {
namespace core {
namespace graphics {

class GraphicsManager {
 public:
  GraphicsManager();

  bool Hook();

  void BroadcastApplicationStats(double frame_time, double fps);

  void Render();
  void OnResize();

  WindowManager *get_window_manager();

  Dx9Hook *get_dx9_hook();
  DxgiHook *get_dxgi_hook();

  StatsCalculator *get_stats_calculator();

  void set_renderer(std::unique_ptr<IGraphicsRenderer> &&renderer);
  std::unique_ptr<IGraphicsRenderer> &get_renderer();

 private:
  WindowManager window_mananger_;

  Dx9Hook dx9_hook_;
  DxgiHook dxgi_hook_;

  std::unique_ptr<IGraphicsRenderer> renderer_;

  StatsCalculator stats_calculator_;
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay