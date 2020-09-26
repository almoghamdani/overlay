#include "client_impl.h"

#include <grpcpp/grpcpp.h>
#include <overlay/error.h>

#include "auth.grpc.pb.h"
#include "utils/token.h"
#include "window_group_impl.h"

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
}

void ClientImpl::SubscribeToEvent(
    EventType event_type,
    std::function<void(std::shared_ptr<Event>)> callback) {
  if (channel_ == nullptr) {
    throw Error(ErrorCode::NotConnected);
  }

  event_manager_->SubscribeToEvent(
      static_cast<EventResponse::EventCase>(event_type),
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
  GUID window_group_id;

  grpc::ClientContext context;
  CreateWindowGroupRequest request;
  CreateWindowGroupResponse response;

  WindowGroupProperties *properties = nullptr;

  // Verify attributes
  if (attributes.opacity < 0 || attributes.opacity > 1) {
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
  request.set_allocated_properties(properties);
  if (!windows_stub_->CreateWindowGroup(&context, request, &response).ok() ||
      response.id().size() != sizeof(window_group_id)) {
    throw Error(ErrorCode::UnknownError);
  }

  // Copy the window id
  std::memcpy(&window_group_id, response.id().data(), sizeof(window_group_id));

  return std::static_pointer_cast<WindowGroup>(
      std::make_shared<WindowGroupImpl>(weak_from_this(), window_group_id,
                                        attributes));
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

std::shared_ptr<Event> ClientImpl::GenerateEvent(
    EventResponse &response) const {
  switch (response.event_case()) {
    case EventResponse::EventCase::kApplicationStats:
      return std::shared_ptr<Event>(
          new ApplicationStatsEvent(response.applicationstats().width(),
                                    response.applicationstats().height(),
                                    response.applicationstats().fullscreen(),
                                    response.applicationstats().frametime(),
                                    response.applicationstats().fps()));

    default:
      return std::shared_ptr<Event>(nullptr);
  }
}

std::unique_ptr<Windows::Stub> &ClientImpl::get_windows_stub() {
  return windows_stub_;
}

}  // namespace helper
}  // namespace overlay
