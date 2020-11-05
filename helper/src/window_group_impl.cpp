#include "window_group_impl.h"

#include <grpcpp/grpcpp.h>
#include <overlay/error.h>

#include <cstdint>

#include "client_impl.h"
#include "windows.grpc.pb.h"

namespace overlay {
namespace helper {

WindowGroup::~WindowGroup() {}

WindowGroupImpl::WindowGroupImpl(std::weak_ptr<ClientImpl> client, GUID id,
                                 const WindowGroupAttributes attributes)
    : client_(client), id_(id), attributes_(attributes) {}

void WindowGroupImpl::SetAttributes(const WindowGroupAttributes attributes) {
  grpc::ClientContext context;
  UpdateWindowGroupPropertiesRequest request;
  UpdateWindowGroupPropertiesResponse response;

  WindowGroupProperties* properties = nullptr;

  std::shared_ptr<ClientImpl> client = client_.lock();
  if (!client) {
    throw Error(ErrorCode::ClientObjectDeallocated);
  }

  // Verify attributes
  if (attributes.opacity < 0 || attributes.opacity > 1 ||
      attributes.buffer_opacity < 0 || attributes.buffer_opacity > 1) {
    throw Error(ErrorCode::InvalidAttributes);
  }

  // Try to update the window group properties
  properties = request.mutable_properties();
  properties->set_z(attributes.z);
  properties->set_opacity(attributes.opacity);
  properties->set_hidden(attributes.hidden);
  properties->set_has_buffer(attributes.has_buffer);
  properties->set_buffer_color(attributes.buffer_color);
  properties->set_buffer_opacity(attributes.buffer_opacity);
  request.set_group_id((const char*)&id_, sizeof(id_));
  if (!client->get_windows_stub()
           ->UpdateWindowGroupProperties(&context, request, &response)
           .ok()) {
    throw Error(ErrorCode::UnknownError);
  }

  // Set the new attributes
  attributes_ = attributes;
}

const WindowGroupAttributes WindowGroupImpl::GetAttributes() const {
  return attributes_;
}

std::shared_ptr<Window> WindowGroupImpl::CreateNewWindow(
    const Rect rect, const WindowAttributes attributes) {
  std::shared_ptr<WindowImpl> window = nullptr;

  GUID window_id;

  grpc::ClientContext context;
  CreateWindowRequest request;
  CreateWindowResponse response;

  WindowRect* window_rect = nullptr;
  WindowProperties* properties = nullptr;

  std::shared_ptr<ClientImpl> client = client_.lock();
  if (!client) {
    throw Error(ErrorCode::ClientObjectDeallocated);
  }

  // Verify attributes
  if (attributes.opacity < 0 || attributes.opacity > 1) {
    throw Error(ErrorCode::InvalidAttributes);
  }

  // Try to create the new window
  properties = request.mutable_properties();
  window_rect = request.mutable_rect();
  properties->set_opacity(attributes.opacity);
  properties->set_hidden(attributes.hidden);
  window_rect->set_height((int)rect.height);
  window_rect->set_width(rect.width);
  window_rect->set_x(rect.x);
  window_rect->set_y(rect.y);
  request.set_group_id((const char*)&id_, sizeof(id_));
  if (!client->get_windows_stub()
           ->CreateWindowInGroup(&context, request, &response)
           .ok() ||
      response.id().size() != sizeof(window_id)) {
    throw Error(ErrorCode::UnknownError);
  }

  // Copy the window id
  std::memcpy(&window_id, response.id().data(), sizeof(window_id));

  window = std::make_shared<WindowImpl>(client_, shared_from_this(), window_id,
                                        id_, rect, attributes);

  {
    std::lock_guard windows_lk(windows_mutex_);
    windows_[window_id] = window;
  }

  return std::static_pointer_cast<Window>(window);
}

std::shared_ptr<WindowImpl> WindowGroupImpl::GetWindowWithId(GUID id) {
  std::shared_ptr<WindowImpl> window = nullptr;

  std::lock_guard windows_lk(windows_mutex_);

  try {
    window = windows_.at(id).lock();
    if (!window) {
      windows_.erase(id);
    }
  } catch (...) {
  }

  return window;
}

}  // namespace helper
}  // namespace overlay