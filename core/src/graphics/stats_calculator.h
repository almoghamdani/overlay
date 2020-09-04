#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

#define INITIAL_FRAME_SAMPLES_CAP 50
#define STATS_CALC_TIME std::chrono::milliseconds(35)

namespace overlay {
namespace core {
namespace graphics {

class StatsCalculator {
 public:
  StatsCalculator();

  void Frame();

 private:
  std::chrono::high_resolution_clock::time_point frame_render_start_;
  std::deque<std::chrono::nanoseconds> frame_times_;
  std::mutex frame_times_mutex_;

  std::chrono::high_resolution_clock::time_point last_calc_time_;
  std::thread calc_thread_;
  std::condition_variable calc_cv_;

  uint64_t frame_samples_cap_;

  void CalculateStats();
};

}  // namespace graphics
}  // namespace core
}  // namespace overlay