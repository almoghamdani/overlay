#include "stats_calculator.h"

#include "core.h"

namespace overlay {
namespace core {
namespace graphics {

StatsCalculator::StatsCalculator()
    : frame_render_start_(std::chrono::high_resolution_clock::now()),
      last_calc_time_(std::chrono::high_resolution_clock::now()),
      calc_thread_(&StatsCalculator::CalculateStats, this) {}

void StatsCalculator::Frame() {
  std::unique_lock lk(frame_times_mutex_);

  // Calculate current frame time
  frame_times_.push_back(std::chrono::high_resolution_clock::now() -
                         frame_render_start_);
  lk.unlock();

  // If we reached the max samples, calculate stats
  if (std::chrono::high_resolution_clock::now() - last_calc_time_ >=
      STATS_CALC_TIME) {
    calc_cv_.notify_one();

    // Reset current sample
    last_calc_time_ = std::chrono::high_resolution_clock::now();
  }

  // Prepare next frame timing
  frame_render_start_ = std::chrono::high_resolution_clock::now();
}

void StatsCalculator::CalculateStats() {
  double avg_frame_time = 0, avg_fps = 0;

  uint64_t frame_time_sum = 0;
  std::deque<std::chrono::nanoseconds> frame_times;

  // TODO: Handle stopping threads
  while (true) {
    std::deque<std::chrono::nanoseconds> new_frame_times;

    // Wait for new data
    std::unique_lock lk(frame_times_mutex_);
    calc_cv_.wait(lk);

    std::swap(frame_times_, new_frame_times);
    lk.unlock();

    for (auto& frame_time : new_frame_times) {
      frame_times.push_back(frame_time);
      frame_time_sum += frame_time.count();
    }

    // Remove old samples
    while (frame_times.size() > FRAME_TIME_SAMPLES) {
      frame_time_sum -= frame_times.front().count();
      frame_times.pop_front();
    }

    avg_frame_time = frame_time_sum / (1000000.0 * frame_times.size());
    avg_fps = 1 / avg_frame_time * 1000.0;

    core::Core::Get()->get_graphics_manager()->BroadcastApplicationStats(
        avg_frame_time, avg_fps);
  }
}

}  // namespace graphics
}  // namespace core
}  // namespace overlay