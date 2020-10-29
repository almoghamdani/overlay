#include "windows_service_impl.h"

#include "core.h"

namespace overlay {
namespace core {
namespace ipc {

grpc::Status WindowsServiceImpl::CreateWindowGroup(
    grpc::ServerContext *context, const CreateWindowGroupRequest *request,
    CreateWindowGroupResponse *response) {
  GUID window_group_id;

  graphics::WindowGroupAttributes attributes;

  attributes.z = (int32_t)request->properties().z();
  attributes.opacity = request->properties().opacity();
  attributes.hidden = request->properties().hidden();
  attributes.has_buffer = request->properties().has_buffer();
  attributes.buffer_color = request->properties().buffer_color();
  attributes.buffer_opacity = request->properties().buffer_opacity();

  // Verify attributes values
  if (attributes.opacity < 0 || attributes.opacity > 1 ||
      attributes.buffer_opacity < 0 || attributes.buffer_opacity > 1) {
    return grpc::Status::CANCELLED;
  }

  // Create the new window group for the client
  window_group_id = Core::Get()
                        ->get_graphics_manager()
                        ->get_window_manager()
                        ->CreateWindowGroup(context->peer(), attributes);

  // Set the window group id
  response->set_id((const char *)&window_group_id, sizeof(window_group_id));

  return grpc::Status::OK;
}

grpc::Status WindowsServiceImpl::UpdateWindowGroupProperties(
    grpc::ServerContext *context,
    const UpdateWindowGroupPropertiesRequest *request,
    UpdateWindowGroupPropertiesResponse *response) {
  GUID window_group_id;

  graphics::WindowGroupAttributes attributes;

  // Verify the client created the window group
  if (request->group_id().size() != sizeof(window_group_id)) {
    return grpc::Status::CANCELLED;
  } else {
    memcpy(&window_group_id, request->group_id().data(),
           sizeof(window_group_id));

    if (Core::Get()
            ->get_graphics_manager()
            ->get_window_manager()
            ->GetWindowGroupClientId(window_group_id) != context->peer()) {
      return grpc::Status::CANCELLED;
    }
  }

  attributes.z = (int32_t)request->properties().z();
  attributes.opacity = request->properties().opacity();
  attributes.hidden = request->properties().hidden();
  attributes.has_buffer = request->properties().has_buffer();
  attributes.buffer_color = request->properties().buffer_color();
  attributes.buffer_opacity = request->properties().buffer_opacity();

  // Verify attributes values
  if (attributes.opacity < 0 || attributes.opacity > 1 ||
      attributes.buffer_opacity < 0 || attributes.buffer_opacity > 1) {
    return grpc::Status::CANCELLED;
  }

  // Update attributes
  if (!Core::Get()
           ->get_graphics_manager()
           ->get_window_manager()
           ->UpdateWindowGroupAttributes(window_group_id, attributes)) {
    return grpc::Status::CANCELLED;
  }

  return grpc::Status::OK;
}

grpc::Status WindowsServiceImpl::CreateWindowInGroup(
    grpc::ServerContext *context, const CreateWindowRequest *request,
    CreateWindowResponse *response) {
  GUID window_group_id, window_id;

  graphics::Rect rect;
  graphics::WindowAttributes attributes;

  // Verify the client created the window group
  if (request->group_id().size() != sizeof(window_group_id)) {
    return grpc::Status::CANCELLED;
  } else {
    memcpy(&window_group_id, request->group_id().data(),
           sizeof(window_group_id));

    if (Core::Get()
            ->get_graphics_manager()
            ->get_window_manager()
            ->GetWindowGroupClientId(window_group_id) != context->peer()) {
      return grpc::Status::CANCELLED;
    }
  }

  rect.width = (size_t)request->rect().width();
  rect.height = (size_t)request->rect().height();
  rect.x = (size_t)request->rect().x();
  rect.y = (size_t)request->rect().y();
  attributes.opacity = request->properties().opacity();
  attributes.hidden = request->properties().hidden();

  // Verify attributes values
  if (attributes.opacity < 0 || attributes.opacity > 1) {
    return grpc::Status::CANCELLED;
  }

  // Create the new window for the client
  window_id = Core::Get()
                  ->get_graphics_manager()
                  ->get_window_manager()
                  ->CreateWindowInGroup(window_group_id, rect, attributes);

  // Set the window id
  response->set_id((const char *)&window_id, sizeof(window_id));

  return grpc::Status::OK;
}

grpc::Status WindowsServiceImpl::UpdateWindowProperties(
    grpc::ServerContext *context, const UpdateWindowPropertiesRequest *request,
    UpdateWindowPropertiesResponse *response) {
  GUID window_group_id, window_id;

  graphics::WindowAttributes attributes;

  // Verify the client created the window group
  if (request->group_id().size() != sizeof(window_group_id)) {
    return grpc::Status::CANCELLED;
  } else {
    memcpy(&window_group_id, request->group_id().data(),
           sizeof(window_group_id));

    if (Core::Get()
            ->get_graphics_manager()
            ->get_window_manager()
            ->GetWindowGroupClientId(window_group_id) != context->peer()) {
      return grpc::Status::CANCELLED;
    }
  }

  attributes.opacity = request->properties().opacity();
  attributes.hidden = request->properties().hidden();

  // Verify attributes values
  if (attributes.opacity < 0 || attributes.opacity > 1) {
    return grpc::Status::CANCELLED;
  }

  // Verify the size of the window id
  if (request->window_id().size() != sizeof(window_id)) {
    return grpc::Status::CANCELLED;
  }
  memcpy(&window_id, request->window_id().data(), sizeof(window_id));

  // Update attributes
  if (!Core::Get()
           ->get_graphics_manager()
           ->get_window_manager()
           ->UpdateWindowAttributes(window_group_id, window_id, attributes)) {
    return grpc::Status::CANCELLED;
  }

  return grpc::Status::OK;
}

grpc::Status WindowsServiceImpl::BufferForWindow(
    grpc::ServerContext *context, const BufferForWindowRequest *request,
    BufferForWindowResponse *response) {
  GUID window_group_id, window_id;

  std::shared_ptr<graphics::Window> window;

  // Verify the client created the window group
  if (request->group_id().size() != sizeof(window_group_id)) {
    return grpc::Status::CANCELLED;
  } else {
    memcpy(&window_group_id, request->group_id().data(),
           sizeof(window_group_id));

    if (Core::Get()
            ->get_graphics_manager()
            ->get_window_manager()
            ->GetWindowGroupClientId(window_group_id) != context->peer()) {
      return grpc::Status::CANCELLED;
    }
  }

  // Verify the size of the window id
  if (request->window_id().size() != sizeof(window_id)) {
    return grpc::Status::CANCELLED;
  }
  memcpy(&window_id, request->window_id().data(), sizeof(window_id));

  // Set the buffer for the window
  Core::Get()
      ->get_graphics_manager()
      ->get_window_manager()
      ->UpdateWindowBufferInGroup(window_group_id, window_id,
                                  std::move((std::string &)request->buffer()));

  return grpc::Status::OK;
}

}  // namespace ipc
}  // namespace core
}  // namespace overlay