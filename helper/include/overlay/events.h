#ifndef OVERLAY_EVENTS_H
#define OVERLAY_EVENTS_H
namespace overlay {
namespace helper {

enum class EventType { ApplicationStats = 1 };

struct Event {
  Event(EventType type) : type(type) {}

  EventType type;
};

struct ApplicationStatsEvent : public Event {
  ApplicationStatsEvent(size_t width, size_t height, bool fullscreen,
                        double frame_time, double fps)
      : Event(EventType::ApplicationStats),
        width(width),
        height(height),
        fullscreen(fullscreen),
        frame_time(frame_time),
        fps(fps) {}

  size_t width;
  size_t height;
  bool fullscreen;
  double frame_time;
  double fps;
};

}  // namespace helper
}  // namespace overlay
#endif