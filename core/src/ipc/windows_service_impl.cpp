#include "windows_service_impl.h"

#include "core.h"
#include "cursors.h"

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

grpc::Status WindowsServiceImpl::SetWindowRect(
    grpc::ServerContext *context, const SetWindowRectRequest *request,
    SetWindowRectResponse *response) {
  GUID window_group_id, window_id;

  graphics::Rect rect;

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

  rect.height = (size_t)request->rect().height();
  rect.width = (size_t)request->rect().width();
  rect.x = (size_t)request->rect().x();
  rect.y = (size_t)request->rect().y();

  // Verify the size of the window id
  if (request->window_id().size() != sizeof(window_id)) {
    return grpc::Status::CANCELLED;
  }
  memcpy(&window_id, request->window_id().data(), sizeof(window_id));

  // Update rect
  if (!Core::Get()->get_graphics_manager()->get_window_manager()->SetWindowRect(
          window_group_id, window_id, rect)) {
    return grpc::Status::CANCELLED;
  }

  return grpc::Status::OK;
}

grpc::Status WindowsServiceImpl::SetWindowCursor(
    grpc::ServerContext *context, const SetWindowCursorRequest *request,
    SetWindowCursorResponse *response) {
  GUID window_group_id, window_id;

  HINSTANCE instance = Core::Get()->get_instance();
  HCURSOR cursor = NULL;

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

  switch (request->cursor()) {
    case Cursor::NONE:
      cursor = NULL;
      break;

    case Cursor::ARROW:
      cursor = LoadCursor(NULL, IDC_ARROW);
      break;

    case Cursor::ARROW_PROGRESS:
      cursor = LoadCursor(NULL, IDC_APPSTARTING);
      break;

    case Cursor::WAIT:
      cursor = LoadCursor(NULL, IDC_WAIT);
      break;

    case Cursor::TEXT:
      cursor = LoadCursor(NULL, IDC_IBEAM);
      break;

    case Cursor::POINTER:
      cursor = LoadCursor(NULL, IDC_HAND);
      break;

    case Cursor::HELP:
      cursor = LoadCursor(NULL, IDC_HELP);
      break;

    case Cursor::CROSSHAIR:
      cursor = LoadCursor(NULL, IDC_CROSS);
      break;

    case Cursor::MOVE:
      cursor = LoadCursor(NULL, IDC_SIZEALL);
      break;

    case Cursor::RESIZE_NESW:
      cursor = LoadCursor(NULL, IDC_SIZENESW);
      break;

    case Cursor::RESIZE_NS:
      cursor = LoadCursor(NULL, IDC_SIZENS);
      break;

    case Cursor::RESIZE_NWSE:
      cursor = LoadCursor(NULL, IDC_SIZENWSE);
      break;

    case Cursor::RESIZE_WE:
      cursor = LoadCursor(NULL, IDC_SIZEWE);
      break;

    case Cursor::NO:
      cursor = LoadCursor(NULL, IDC_NO);
      break;

    case Cursor::ALIAS:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_ALIAS));
      break;

    case Cursor::CELL:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_CELL));
      break;

    case Cursor::COL_RESIZE:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_COLRESIZE));
      break;

    case Cursor::GRAB:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_HAND_GRAB));
      break;

    case Cursor::GRABBING:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_HAND_GRABBING));
      break;

    case Cursor::PAN_E:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_PAN_EAST));
      break;

    case Cursor::PAN_M:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_PAN_MIDDLE));
      break;

    case Cursor::PAN_MH:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_PAN_MIDDLE_HORIZONTAL));
      break;

    case Cursor::PAN_MV:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_PAN_MIDDLE_VERTICAL));
      break;

    case Cursor::PAN_N:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_PAN_NORTH));
      break;

    case Cursor::PAN_NE:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_PAN_NORTH_EAST));
      break;

    case Cursor::PAN_NW:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_PAN_NORTH_WEST));
      break;

    case Cursor::PAN_S:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_PAN_SOUTH));
      break;

    case Cursor::PAN_SE:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_PAN_SOUTH_EAST));
      break;

    case Cursor::PAN_SW:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_PAN_SOUTH_WEST));
      break;

    case Cursor::PAN_W:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_PAN_WEST));
      break;

    case Cursor::ROW_RESIZE:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_ROWRESIZE));
      break;

    case Cursor::VERTICAL_TEXT:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_VERTICALTEXT));
      break;

    case Cursor::ZOOM_IN:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_ZOOMIN));
      break;

    case Cursor::ZOOM_OUT:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_ZOOMOUT));
      break;

    case Cursor::COPY:
      cursor = LoadCursor(instance, MAKEINTRESOURCE(IDC_COPYCUR));
      break;

    default:
      return grpc::Status::CANCELLED;
  }

  // Verify the size of the window id
  if (request->window_id().size() != sizeof(window_id)) {
    return grpc::Status::CANCELLED;
  }
  memcpy(&window_id, request->window_id().data(), sizeof(window_id));

  // Update cursor
  if (!Core::Get()
           ->get_graphics_manager()
           ->get_window_manager()
           ->SetWindowCursor(window_group_id, window_id, cursor)) {
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