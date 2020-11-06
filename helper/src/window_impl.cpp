#include "window_impl.h"

#include <grpcpp/grpcpp.h>
#include <overlay/error.h>

#include <cstdint>
#include <magic_enum.hpp>

#include "client_impl.h"
#include "window_group_impl.h"
#include "windows.grpc.pb.h"

namespace overlay {
namespace helper {

Window::~Window() {}

WindowImpl::WindowImpl(std::weak_ptr<ClientImpl> client,
                       std::shared_ptr<WindowGroupImpl> window_group, GUID id,
                       GUID group_id, const Rect rect,
                       const WindowAttributes attributes)
    : client_(client),
      window_group_(window_group),
      id_(id),
      group_id_(group_id),
      rect_(rect),
      attributes_(attributes),
      cursor_(Cursor::Arrow) {}

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
  properties = request.mutable_properties();
  properties->set_opacity(attributes.opacity);
  properties->set_hidden(attributes.hidden);
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

void WindowImpl::SetRect(const Rect rect) {
  grpc::ClientContext context;
  SetWindowRectRequest request;
  SetWindowRectResponse response;

  WindowRect* window_rect = nullptr;

  std::shared_ptr<ClientImpl> client = client_.lock();
  if (!client) {
    throw Error(ErrorCode::ClientObjectDeallocated);
  }

  // Try to set the rect
  window_rect = request.mutable_rect();
  window_rect->set_height(rect.height);
  window_rect->set_width(rect.width);
  window_rect->set_x(rect.x);
  window_rect->set_y(rect.y);
  request.set_group_id((const char*)&group_id_, sizeof(group_id_));
  request.set_window_id((const char*)&id_, sizeof(id_));
  if (!client->get_windows_stub()
           ->SetWindowRect(&context, request, &response)
           .ok()) {
    throw Error(ErrorCode::UnknownError);
  }

  // Set the new rect
  rect_ = rect;
}

const Rect WindowImpl::GetRect() const { return rect_; }

void WindowImpl::SetCursor(const Cursor cursor) {
  grpc::ClientContext context;
  SetWindowCursorRequest request;
  SetWindowCursorResponse response;

  std::shared_ptr<ClientImpl> client = client_.lock();
  if (!client) {
    throw Error(ErrorCode::ClientObjectDeallocated);
  }

  if (!magic_enum::enum_contains<Cursor>(cursor)) {
    throw Error(ErrorCode::InvalidCursor);
  }

  // Try to set the cursor
  request.set_cursor((overlay::Cursor)cursor);
  request.set_group_id((const char*)&group_id_, sizeof(group_id_));
  request.set_window_id((const char*)&id_, sizeof(id_));
  if (!client->get_windows_stub()
           ->SetWindowCursor(&context, request, &response)
           .ok()) {
    throw Error(ErrorCode::UnknownError);
  }

  // Set the new rect
  cursor_ = cursor;
}

const Cursor WindowImpl::GetCursor() const { return cursor_; }

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
  if (buffer_size != rect_.width * rect_.height * sizeof(uint32_t)) {
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
  if (!magic_enum::enum_contains<WindowEventType>(event_type)) {
    throw Error(ErrorCode::InvalidEventType);
  }

  std::lock_guard event_handlers_lk(event_handlers_mutex_);
  event_handlers_[event_type] = callback;
}

void WindowImpl::UnsubscribeEvent(WindowEventType event_type) {
  if (!magic_enum::enum_contains<WindowEventType>(event_type)) {
    throw Error(ErrorCode::InvalidEventType);
  }

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
  WindowEvent* window_event = nullptr;

  switch (event.event_case()) {
    case EventResponse::WindowEvent::EventCase::kKeyboardInputEvent:
      window_event =
          event.keyboardinputevent().type() ==
                  EventResponse::WindowEvent::KeyboardInputEvent::CHAR
              ? new WindowKeyboardInputEvent(
                    (WindowKeyboardInputEvent::InputType)event
                        .keyboardinputevent()
                        .type(),
                    (wchar_t)event.keyboardinputevent().code())
              : new WindowKeyboardInputEvent(
                    (WindowKeyboardInputEvent::InputType)event
                        .keyboardinputevent()
                        .type(),
                    (WindowKeyboardInputEvent::KeyCode)event
                        .keyboardinputevent()
                        .code());
      break;

    case EventResponse::WindowEvent::EventCase::kMouseInputEvent:
      switch (event.mouseinputevent().type()) {
        case EventResponse::WindowEvent::MouseInputEvent::MOUSE_MOVE:
          window_event = new WindowMouseInputEvent(
              (WindowMouseInputEvent::InputType)event.mouseinputevent().type(),
              (size_t)event.mouseinputevent().x(),
              (size_t)event.mouseinputevent().y());
          break;

        case EventResponse::WindowEvent::MouseInputEvent::MOUSE_VERTICAL_WHEEL:
        case EventResponse::WindowEvent::MouseInputEvent::
            MOUSE_HORIZONTAL_WHEEL:
          window_event = new WindowMouseInputEvent(
              (WindowMouseInputEvent::InputType)event.mouseinputevent().type(),
              (size_t)event.mouseinputevent().x(),
              (size_t)event.mouseinputevent().y(),
              (int)event.mouseinputevent().wheeldelta());
          break;

        case EventResponse::WindowEvent::MouseInputEvent::MOUSE_BUTTON_DOWN:
        case EventResponse::WindowEvent::MouseInputEvent::MOUSE_BUTTON_UP:
        case EventResponse::WindowEvent::MouseInputEvent::
            MOUSE_BUTTON_DOUBLE_CLICK:
          window_event = new WindowMouseInputEvent(
              (WindowMouseInputEvent::InputType)event.mouseinputevent().type(),
              (size_t)event.mouseinputevent().x(),
              (size_t)event.mouseinputevent().y(),
              (WindowMouseInputEvent::Button)event.mouseinputevent().button());
          break;

        default:
          break;
      }

      break;

    case EventResponse::WindowEvent::EventCase::kFocusEvent:
      window_event = new WindowFocusEvent();
      break;

    case EventResponse::WindowEvent::EventCase::kBlurEvent:
      window_event = new WindowBlurEvent();
      break;

    default:
      break;
  }

  return std::shared_ptr<WindowEvent>(window_event);
}

}  // namespace helper
}  // namespace overlay