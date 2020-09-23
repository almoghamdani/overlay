#include "windows_service_impl.h"

#include "core.h"

namespace overlay {
namespace core {
namespace ipc {

grpc::Status WindowsServiceImpl::CreateNewWindow(
    grpc::ServerContext *context, const CreateWindowRequest *request,
    CreateWindowResponse *response) {
  GUID window_id;

  graphics::Rect rect;

  rect.width = (size_t)request->rect().width();
  rect.height = (size_t)request->rect().height();
  rect.x = (size_t)request->rect().x();
  rect.y = (size_t)request->rect().y();

  // Create the new window for the client
  window_id =
      Core::Get()
          ->get_graphics_manager()
          ->get_window_manager()
          ->CreateWindowForClient(context->peer(), rect, request->rect().z());

  // Set the window id
  response->set_id((const char *)&window_id, sizeof(window_id));

  return grpc::Status::OK;
}

grpc::Status WindowsServiceImpl::BufferForWindow(
    grpc::ServerContext *context, const BufferForWindowRequest *request,
    BufferForWindowResponse *response) {
  GUID window_id;
  std::vector<uint8_t> buffer;

  std::shared_ptr<graphics::Window> window;

  // Verify the size of the window id
  if (request->id().size() != sizeof(window_id)) {
    return grpc::Status::CANCELLED;
  }
  memcpy(&window_id, request->id().data(), sizeof(window_id));

  // Verify the owner of the window
  if (!(window = Core::Get()
                     ->get_graphics_manager()
                     ->get_window_manager()
                     ->GetWindowWithId(window_id)) ||
      window->client_id != context->peer()) {
    return grpc::Status::CANCELLED;
  }

  // Copy the buffer of the window
  buffer.assign(request->buffer().begin(), request->buffer().end());

  // Set the buffer for the window
  Core::Get()->get_graphics_manager()->get_window_manager()->SwapWindowBuffer(
      window_id, buffer);

  return grpc::Status::OK;
}

}  // namespace ipc
}  // namespace core
}  // namespace overlay