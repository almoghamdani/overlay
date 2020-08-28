#ifndef OVERLAY_EVENTS_H
#define OVERLAY_EVENTS_H
namespace overlay {
namespace helper {

enum class EventType { Fps = 1 };

struct Event {
  Event(EventType type) : type(type) {}

  EventType type;
};

struct FpsEvent : public Event {
  FpsEvent(double fps) : Event(EventType::Fps), fps(fps) {}

  double fps;
};

}  // namespace helper
}  // namespace overlay
#endif