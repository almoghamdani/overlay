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
  ApplicationStatsEvent(double frame_time, double fps)
      : Event(EventType::ApplicationStats), frame_time(frame_time), fps(fps) {}

  double frame_time;
  double fps;
};

}  // namespace helper
}  // namespace overlay
#endif