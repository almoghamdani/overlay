#include "client_impl.h"

#include <grpcpp/grpcpp.h>
#include <overlay/error.h>

#include "auth.grpc.pb.h"
#include "utils/token.h"

namespace overlay {
namespace helper {

Client::~Client() {}

ClientImpl::ClientImpl(DWORD process_id)
    : overlay_pid_(process_id),
      channel_(nullptr),
      windows_stub_(nullptr),
      event_manager_(nullptr) {}

std::shared_ptr<Client> CreateClient(DWORD process_id) {
  return std::static_pointer_cast<Client>(
      std::make_shared<ClientImpl>(process_id));
}

void ClientImpl::Connect() {
  // Check that the client isn't already connected to a client
  if (channel_ != nullptr) {
    throw Error(ErrorCode::AlreadyConnected);
  }

  // Connect to the server
  channel_ = ConnectToServerChannel();

  // Create the windows stub
  windows_stub_ = Windows::NewStub(channel_);

  // Create event manager and start it
  event_manager_ = std::make_unique<EventManager>(channel_);
  event_manager_->StartHandlingAsyncRpcs();

  // Set handler for window event
  event_manager_->SubscribeToEvent(
      EventResponse::EventCase::kWindowEvent,
      [this](EventResponse &res) { HandleWindowEvent(res); });
}

void ClientImpl::SubscribeToEvent(
    EventType event_type,
    std::function<void(std::shared_ptr<Event>)> callback) {
  EventResponse::EventCase type = ConvertEventType(event_type);

  if (type == EventResponse::EventCase::EVENT_NOT_SET) {
    return;
  }

  if (channel_ == nullptr) {
    throw Error(ErrorCode::NotConnected);
  }

  event_manager_->SubscribeToEvent(type,
                                   [this, callback](EventResponse &response) {
                                     callback(GenerateEvent(response));
                                   });
}

void ClientImpl::UnsubscribeEvent(EventType event_type) {
  event_manager_->UnsubscribeEvent(
      static_cast<EventResponse::EventCase>(event_type));
}

std::shared_ptr<WindowGroup> ClientImpl::CreateWindowGroup(
    const WindowGroupAttributes attributes) {
  std::shared_ptr<WindowGroupImpl> window_group = nullptr;

  GUID window_group_id;

  grpc::ClientContext context;
  CreateWindowGroupRequest request;
  CreateWindowGroupResponse response;

  WindowGroupProperties *properties = nullptr;

  // Verify attributes
  if (attributes.opacity < 0 || attributes.opacity > 1 ||
      attributes.buffer_opacity < 0 || attributes.buffer_opacity > 1) {
    throw Error(ErrorCode::InvalidAttributes);
  }

  // If the client isn't connected
  if (windows_stub_ == nullptr) {
    throw Error(ErrorCode::NotConnected);
  }

  // Try to create the new window
  properties = new WindowGroupProperties();  // This should be deallocated by
                                             // the request itself since we're
                                             // using `set_allocated_properties`
  properties->set_z(attributes.z);
  properties->set_opacity(attributes.opacity);
  properties->set_hidden(attributes.hidden);
  properties->set_has_buffer(attributes.has_buffer);
  properties->set_buffer_color(attributes.buffer_color);
  properties->set_buffer_opacity(attributes.buffer_opacity);
  request.set_allocated_properties(properties);
  if (!windows_stub_->CreateWindowGroup(&context, request, &response).ok() ||
      response.id().size() != sizeof(window_group_id)) {
    throw Error(ErrorCode::UnknownError);
  }

  // Copy the window id
  std::memcpy(&window_group_id, response.id().data(), sizeof(window_group_id));

  window_group = std::make_shared<WindowGroupImpl>(weak_from_this(),
                                                   window_group_id, attributes);

  {
    std::lock_guard window_groups_lk(window_groups_mutex_);
    window_groups_[window_group_id] = window_group;
  }

  return std::static_pointer_cast<WindowGroup>(window_group);
}

AuthenticateResponse ClientImpl::GetAuthInfo() const {
  AuthenticateResponse res;

  HANDLE pipe =
      CreateFileA(utils::token::GeneratePipeName(overlay_pid_).c_str(),
                  GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
  DWORD read = 0;

  if (!pipe) {
    throw Error(ErrorCode::ProcessNotFound);
  }

  // Read the authenticate response from the server
  ReadFile(pipe, &res, sizeof(res), &read, NULL);
  CloseHandle(pipe);

  // Verify response size
  if (read != sizeof(res)) {
    throw Error(ErrorCode::UnknownError);
  }

  return res;
}

std::shared_ptr<grpc::Channel> ClientImpl::ConnectToServerChannel() const {
  std::shared_ptr<grpc::Channel> channel(nullptr);

  TokenAuthenticationRequest token_auth_req;
  TokenAuthenticationResponse token_auth_res;
  std::unique_ptr<Authentication::Stub> auth_stub(nullptr);
  grpc::ClientContext context;

  AuthenticateResponse auth = GetAuthInfo();

  grpc::SslCredentialsOptions ssl_options;
  ssl_options.pem_root_certs = auth.server_certificate;

  // Create channel
  channel = grpc::CreateChannel(FormatServerUrl(auth.rpc_server_port),
                                grpc::SslCredentials(ssl_options));

  // Create authentication stub
  auth_stub = Authentication::NewStub(channel);

  // Authenticate with the server
  token_auth_req.set_token((const char *)&auth.token, sizeof(auth.token));
  if (!auth_stub
           ->AuthenticateWithToken(&context, token_auth_req, &token_auth_res)
           .ok()) {
    throw Error(ErrorCode::AuthFailed);
  }

  return channel;
}

std::string ClientImpl::FormatServerUrl(uint16_t port) const {
  std::stringstream ss;

  ss << "localhost:" << port;

  return ss.str();
}

EventResponse::EventCase ClientImpl::ConvertEventType(EventType type) const {
  switch (type) {
    case EventType::ApplicationStats:
      return EventResponse::EventCase::kApplicationStatsEvent;

    default:
      return EventResponse::EventCase::EVENT_NOT_SET;
  }
}

std::shared_ptr<Event> ClientImpl::GenerateEvent(
    EventResponse &response) const {
  switch (response.event_case()) {
    case EventResponse::EventCase::kApplicationStatsEvent:
      return std::shared_ptr<Event>(new ApplicationStatsEvent(
          response.applicationstatsevent().width(),
          response.applicationstatsevent().height(),
          response.applicationstatsevent().fullscreen(),
          response.applicationstatsevent().frametime(),
          response.applicationstatsevent().fps()));

    default:
      return nullptr;
  }
}

void ClientImpl::HandleWindowEvent(EventResponse &response) {
  const EventResponse::WindowEvent &window_event = response.windowevent();

  GUID window_group_id, window_id;

  std::shared_ptr<WindowGroupImpl> window_group = nullptr;
  std::shared_ptr<WindowImpl> window = nullptr;

  // Verify GUID sizes
  if (window_event.windowgroupid().size() != sizeof(window_group_id) ||
      window_event.windowid().size() != sizeof(window_id)) {
    return;
  }

  // Copy window group id and window id
  memcpy(&window_group_id, window_event.windowgroupid().data(),
         sizeof(window_group_id));
  memcpy(&window_id, window_event.windowid().data(), sizeof(window_id));

  try {
    std::lock_guard window_groups_lk(window_groups_mutex_);

    window_group = window_groups_.at(window_group_id).lock();
    if (!window_group) {
      window_groups_.erase(window_group_id);
    }
  } catch (...) {
    return;
  }

  // Try get the window object
  if (!(window = window_group->GetWindowWithId(window_id))) {
    return;
  }

  window->HandleWindowEvent(window_event);
}

std::unique_ptr<Windows::Stub> &ClientImpl::get_windows_stub() {
  return windows_stub_;
}

}  // namespace helper
}  // namespace overlay
