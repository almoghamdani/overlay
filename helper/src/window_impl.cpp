#include "window_impl.h"

#include <grpcpp/grpcpp.h>
#include <overlay/error.h>

#include <cstdint>

#include "client_impl.h"
#include "window_group_impl.h"
#include "windows.grpc.pb.h"

namespace overlay {
namespace helper {

Window::~Window() {}

WindowImpl::WindowImpl(std::weak_ptr<ClientImpl> client,
                       std::shared_ptr<WindowGroupImpl> window_group, GUID id,
                       GUID group_id, const WindowAttributes attributes)
    : client_(client),
      window_group_(window_group),
      id_(id),
      group_id_(group_id),
      attributes_(attributes) {}

void WindowImpl::SetAttributes(const WindowAttributes attributes) {
  grpc::ClientContext context;
  UpdateWindowPropertiesRequest request;
  UpdateWindowPropertiesResponse response;

  WindowProperties* properties = nullptr;

  std::shared_ptr<ClientImpl> client = client_.lock();
  if (!client) {
    throw Error(ErrorCode::ClientObjectDeallocated);
  }

  // Verify attributes
  if (attributes.opacity < 0 || attributes.opacity > 1) {
    throw Error(ErrorCode::InvalidAttributes);
  }

  // Try to update the attributes
  properties = new WindowProperties();  // This should be deallocated by the
                                        // request itself since we're using
                                        // `set_allocated_properties`
  properties->set_width(attributes.width);
  properties->set_height(attributes.height);
  properties->set_x(attributes.x);
  properties->set_y(attributes.y);
  properties->set_opacity(attributes.opacity);
  properties->set_hidden(attributes.hidden);
  request.set_allocated_properties(properties);
  request.set_group_id((const char*)&group_id_, sizeof(group_id_));
  request.set_window_id((const char*)&id_, sizeof(id_));
  if (!client->get_windows_stub()
           ->UpdateWindowProperties(&context, request, &response)
           .ok()) {
    throw Error(ErrorCode::UnknownError);
  }

  // Set the new attributes
  attributes_ = attributes;
}

const WindowAttributes WindowImpl::GetAttributes() const { return attributes_; }

void WindowImpl::UpdateBitmapBuffer(const void* buffer, size_t buffer_size) {
  grpc::ClientContext context;
  BufferForWindowRequest request;
  BufferForWindowResponse response;

  std::shared_ptr<ClientImpl> client = client_.lock();
  if (!client) {
    throw Error(ErrorCode::ClientObjectDeallocated);
  }

  // If the window isn't visible, return
  if (attributes_.hidden || attributes_.opacity == 0 ||
      window_group_->GetAttributes().hidden ||
      window_group_->GetAttributes().opacity == 0) {
    return;
  }

  // Verify buffer size
  if (buffer_size !=
      attributes_.width * attributes_.height * sizeof(uint32_t)) {
    throw Error(ErrorCode::InvalidBitmapBufferSize);
  }

  // Fill the request
  request.set_group_id((const char*)&group_id_, sizeof(group_id_));
  request.set_window_id((const char*)&id_, sizeof(id_));
  request.set_buffer(buffer, buffer_size);

  // Send the buffer to the overlay
  if (!client->get_windows_stub()
           ->BufferForWindow(&context, request, &response)
           .ok()) {
    throw Error(ErrorCode::UnknownError);
  }
}

void WindowImpl::SubscribeToEvent(
    WindowEventType event_type,
    std::function<void(std::shared_ptr<WindowEvent>)> callback) {
  std::lock_guard event_handlers_lk(event_handlers_mutex_);
  event_handlers_[event_type] = callback;
}

void WindowImpl::UnsubscribeEvent(WindowEventType event_type) {
  std::lock_guard event_handlers_lk(event_handlers_mutex_);

  try {
    event_handlers_.erase(event_type);
  } catch (...) {
  }
}

void WindowImpl::HandleWindowEvent(const EventResponse::WindowEvent& event) {
  std::shared_ptr<WindowEvent> window_event = GenerateEvent(event);

  std::function<void(std::shared_ptr<WindowEvent>)> handler = nullptr;

  if (window_event) {
    std::lock_guard event_handlers_lk(event_handlers_mutex_);

    try {
      handler = event_handlers_.at(window_event->type);
    } catch (...) {
      return;
    }
  }

  if (handler) {
    handler(window_event);
  }
}

std::shared_ptr<WindowEvent> WindowImpl::GenerateEvent(
    const EventResponse::WindowEvent& event) const {
  switch (event.event_case()) {
    case EventResponse::WindowEvent::EventCase::kKeyboardInput:
      return std::shared_ptr<WindowEvent>(
          event.keyboardinput().type() ==
                  EventResponse::WindowEvent::KeyboardInputEvent::CHAR
              ? new WindowKeyboardInputEvent(
                    (WindowKeyboardInputEvent::InputType)event.keyboardinput()
                        .type(),
                    (wchar_t)event.keyboardinput().code())
              : new WindowKeyboardInputEvent(
                    (WindowKeyboardInputEvent::InputType)event.keyboardinput()
                        .type(),
                    (WindowKeyboardInputEvent::KeyCode)event.keyboardinput()
                        .code()));

    default:
      return nullptr;
  }
}

}  // namespace helper
}  // namespace overlay