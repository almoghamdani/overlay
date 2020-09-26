#include "window_impl.h"

#include <grpcpp/grpcpp.h>
#include <overlay/error.h>

#include <cstdint>

#include "window_group_impl.h"
#include "windows.grpc.pb.h"

namespace overlay {
namespace helper {

Window::~Window() {}

WindowImpl::WindowImpl(std::weak_ptr<ClientImpl> client,
                       std::shared_ptr<WindowGroupImpl> window_group, GUID id,
                       GUID group_id, const WindowAttributes& attributes)
    : client_(client),
      window_group_(window_group),
      id_(id),
      group_id_(group_id),
      attributes_(attributes) {}

const WindowAttributes WindowImpl::GetAttributes() const { return attributes_; }

void WindowImpl::UpdateBitmapBuffer(const void* buffer, size_t buffer_size) {
  grpc::ClientContext context;
  BufferForWindowRequest request;
  BufferForWindowResponse response;

  // If the window isn't visible, return
  if (attributes_.hidden || attributes_.opacity == 0 ||
      window_group_->GetAttributes().hidden ||
      window_group_->GetAttributes().opacity == 0) {
    return;
  }

  std::shared_ptr<ClientImpl> client = client_.lock();
  if (!client) {
    throw Error(ErrorCode::ClientObjectDeallocated);
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

}  // namespace helper
}  // namespace overlay