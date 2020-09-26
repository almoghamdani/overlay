#include "window_group_impl.h"

#include <grpcpp/grpcpp.h>
#include <overlay/error.h>

#include <cstdint>

#include "window_impl.h"
#include "windows.grpc.pb.h"

namespace overlay {
namespace helper {

WindowGroup::~WindowGroup() {}

WindowGroupImpl::WindowGroupImpl(std::weak_ptr<ClientImpl> client, GUID id,
                                 const WindowGroupAttributes& attributes)
    : client_(client), id_(id), attributes_(attributes) {}

const WindowGroupAttributes WindowGroupImpl::GetAttributes() const {
  return attributes_;
}

std::shared_ptr<Window> WindowGroupImpl::CreateNewWindow(
    const WindowAttributes& attributes) {
  GUID window_id;

  grpc::ClientContext context;
  CreateWindowRequest request;
  CreateWindowResponse response;

  WindowProperties* properties = nullptr;

  std::shared_ptr<ClientImpl> client = client_.lock();
  if (!client) {
    throw Error(ErrorCode::ClientObjectDeallocated);
  }

  // Try to create the new window
  properties = new WindowProperties();  // This should be deallocated by the
                                        // request itself since we're using
                                        // `set_allocated_properties`
  properties->set_width(attributes.width);
  properties->set_height(attributes.height);
  properties->set_x(attributes.x);
  properties->set_y(attributes.y);
  properties->set_z(attributes.z);
  properties->set_opacity(attributes.opacity);
  properties->set_hidden(attributes.hidden);
  request.set_allocated_properties(properties);
  request.set_group_id((const char*)&id_, sizeof(id_));
  if (!client->get_windows_stub()
           ->CreateWindowInGroup(&context, request, &response)
           .ok() ||
      response.id().size() != sizeof(window_id)) {
    throw Error(ErrorCode::UnknownError);
  }

  // Copy the window id
  std::memcpy(&window_id, response.id().data(), sizeof(window_id));

  return std::static_pointer_cast<Window>(std::make_shared<WindowImpl>(
      client_, shared_from_this(), window_id, id_, attributes));
}

}  // namespace helper
}  // namespace overlay