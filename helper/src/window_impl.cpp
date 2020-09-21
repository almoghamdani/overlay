#include "window_impl.h"

#include <grpcpp/grpcpp.h>
#include <overlay/error.h>

#include "windows.grpc.pb.h"

namespace overlay {
namespace helper {

Window::~Window() {}

WindowImpl::WindowImpl(std::weak_ptr<ClientImpl> client, GUID id,
                       const WindowProperties& properties)
    : client_(client), id_(id), properties_(properties) {}

void WindowImpl::UpdateBitmapBuffer(std::vector<uint8_t>& buffer) {
  grpc::ClientContext context;
  BufferForWindowRequest request;
  BufferForWindowResponse response;

  std::shared_ptr<ClientImpl> client = client_.lock();
  if (!client) {
    throw Error(ErrorCode::ClientObjectDeallocated);
  }

  // Verify buffer size
  if (buffer.size() != properties_.width * properties_.height * 4) {
    throw Error(ErrorCode::InvalidBitmapBufferSize);
  }

  // Fill the request
  request.set_id((const char*)&id_, sizeof(id_));
  request.set_buffer((const char*)buffer.data(), buffer.size());

  // Send the buffer to the overlay
  if (!client->get_windows_stub()
           ->BufferForWindow(&context, request, &response)
           .ok()) {
    throw Error(ErrorCode::UnknownError);
  }
}

}  // namespace helper
}  // namespace overlay