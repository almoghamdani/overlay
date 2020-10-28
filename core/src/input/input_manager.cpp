#include "input_manager.h"

#include <loguru/loguru.hpp>

#include "core.h"
#include "events.pb.h"
#include "graphics/window.h"

#define KEY_UP_PASSTHROUGH_LPARAM ((LPARAM)0xA1B3C5D7)

namespace overlay {
namespace core {
namespace input {

LRESULT CALLBACK WindowGetMsgHook(_In_ int code, _In_ WPARAM word_param,
                                  _In_ LPARAM long_param) {
  return Core::Get()->get_input_manager()->WindowMsgHook(code, word_param,
                                                         long_param);
}

InputManager::InputManager()
    : block_app_input_(false), window_msg_hook_(NULL) {}

bool InputManager::Hook() {
  if (!input_hook_.Hook()) {
    DLOG_F(ERROR, "Unable to hook input WinAPI functions!");
    return false;
  } else {
    DLOG_F(INFO, "Hooked input WinAPI functions successfully!");
  }

  return true;
}

void InputManager::ReleasePressedKeys() {
  for (int virtual_key = 0; virtual_key < 256; virtual_key++) {
    if (GetAsyncKeyState(virtual_key) & (1 << 15)) {
      PostMessageA(Core::Get()->get_graphics_window(), WM_KEYUP,
                   (WPARAM)virtual_key, KEY_UP_PASSTHROUGH_LPARAM);
    }
  }
}

uint16_t InputManager::VirtualKeyToScanCode(uint8_t virtual_key) {
  uint16_t scan_code = MapVirtualKey(virtual_key, MAPVK_VK_TO_VSC);

  switch (virtual_key) {
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
    case VK_RCONTROL:
    case VK_RMENU:
    case VK_LWIN:
    case VK_RWIN:
    case VK_APPS:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_END:
    case VK_HOME:
    case VK_INSERT:
    case VK_DELETE:
    case VK_DIVIDE:
    case VK_NUMLOCK:
      scan_code |= KF_EXTENDED;

    default:
      break;
  }

  return scan_code;
}

void InputManager::SaveCursorState() {
  std::lock_guard cursor_state_lk(cursor_state_mutex_);

  cursor_state_.cursor_count =
      input_hook_.show_cursor_hook_.get_trampoline().CallStdMethod<int>(true) -
      1;
  input_hook_.show_cursor_hook_.get_trampoline().CallStdMethod<int>(false);

  cursor_state_.cursor_handle =
      input_hook_.get_cursor_hook_.get_trampoline().CallStdMethod<HCURSOR>();

  input_hook_.get_cursor_pos_hook_.get_trampoline().CallStdMethod<BOOL>(
      &cursor_state_.cursor_pos);

  DLOG_F(INFO,
         "Saved cursor state - Count: %d, Position: (%d, %d), Handle: 0x%x.",
         cursor_state_.cursor_count, cursor_state_.cursor_pos.x,
         cursor_state_.cursor_pos.y, cursor_state_.cursor_handle);
}

void InputManager::RestoreCursorState() {
  std::lock_guard cursor_state_lk(cursor_state_mutex_);

  int cursor_count =
      input_hook_.show_cursor_hook_.get_trampoline().CallStdMethod<int>(false);
  bool show_cursor = cursor_count < cursor_state_.cursor_count;

  // Restore cursor count
  while (cursor_count != cursor_state_.cursor_count) {
    cursor_count =
        input_hook_.show_cursor_hook_.get_trampoline().CallStdMethod<int>(
            show_cursor);
  }

  input_hook_.set_cursor_hook_.get_trampoline().CallStdMethod<HCURSOR>(
      cursor_state_.cursor_handle);
  input_hook_.set_cursor_pos_hook_.get_trampoline().CallStdMethod<BOOL>(
      cursor_state_.cursor_pos.x, cursor_state_.cursor_pos.y);

  DLOG_F(INFO,
         "Restored cursor state - Count: %d, Position: (%d, %d), Handle: 0x%x.",
         cursor_state_.cursor_count, cursor_state_.cursor_pos.x,
         cursor_state_.cursor_pos.y, cursor_state_.cursor_handle);
}

void InputManager::HandleInput(UINT message, uint32_t param) {
  std::shared_ptr<graphics::Window> focused_window =
      Core::Get()
          ->get_graphics_manager()
          ->get_window_manager()
          ->GetFocusedWindow();

  EventResponse event;
  EventResponse::WindowEvent *window_event = nullptr;
  EventResponse::WindowEvent::KeyboardInputEvent *input_event = nullptr;

  if (focused_window) {
    window_event = new EventResponse::WindowEvent();
    input_event = new EventResponse::WindowEvent::KeyboardInputEvent();

    window_event->set_windowgroupid((const char *)&focused_window->group_id,
                                    sizeof(focused_window->group_id));
    window_event->set_windowid((const char *)&focused_window->id,
                               sizeof(focused_window->id));

    input_event->set_code(param);

    switch (message) {
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
        input_event->set_type(
            EventResponse::WindowEvent::KeyboardInputEvent::KEY_DOWN);
        break;

      case WM_CHAR:
      case WM_SYSCHAR:
        input_event->set_type(
            EventResponse::WindowEvent::KeyboardInputEvent::CHAR);
        break;

      case WM_KEYUP:
      case WM_SYSKEYUP:
        input_event->set_type(
            EventResponse::WindowEvent::KeyboardInputEvent::KEY_UP);
        break;

      default:
        break;
    }

    window_event->set_allocated_keyboardinput(input_event);
    event.set_allocated_windowevent(window_event);
    Core::Get()->get_rpc_server()->get_events_service()->BroadcastEvent(event);
  }
}

bool InputManager::HookWindow(HWND window) {
  DWORD thread = GetWindowThreadProcessId(window, NULL);

  window_msg_hook_ =
      SetWindowsHookExW(WH_GETMESSAGE, WindowGetMsgHook, NULL, thread);

  return window_msg_hook_ != NULL;
}

LRESULT InputManager::WindowMsgHook(_In_ int code, _In_ WPARAM word_param,
                                    _In_ LPARAM long_param) {
  MSG *message = NULL;

  if (code >= 0) {
    message = (MSG *)long_param;

    // Check that the message is for the correct window
    if (message->hwnd == Core::Get()->get_graphics_window() &&
        word_param == PM_REMOVE) {
      if (message->message >= WM_KEYFIRST && message->message <= WM_KEYLAST) {
        // Pass-throguh key-up for application
        if (message->message == WM_KEYUP &&
            message->lParam == KEY_UP_PASSTHROUGH_LPARAM) {
          message->lParam = (LPARAM)(
              (1 << 31) + (1 << 30) +
              (VirtualKeyToScanCode((uint8_t)message->wParam) << 16) + 1);
        } else if (block_app_input_) {
          // Translate virtual key to char
          if (message->message == WM_KEYDOWN ||
              message->message == WM_SYSKEYDOWN) {
            TranslateMessage(message);
          }

          // Handle input
          if (message->message == WM_KEYDOWN ||
              message->message == WM_SYSKEYDOWN ||
              message->message == WM_CHAR || message->message == WM_SYSCHAR ||
              message->message == WM_KEYUP || message->message == WM_SYSKEYUP) {
            HandleInput(message->message, (uint32_t)message->wParam);
          }

          // Block application input
          message->message = WM_NULL;
        }
      }

      if (message->message >= WM_MOUSEFIRST &&
          message->message <= WM_MOUSELAST) {
        if (block_app_input_) {
          message->message = WM_NULL;
        }
      }

      // Block raw input (mouse and keyboard)
      if (message->message == WM_INPUT && block_app_input_) {
        message->message = WM_NULL;
      }
    }
  }

  return CallNextHookEx(window_msg_hook_, code, word_param, long_param);
}

bool InputManager::get_block_app_input() const { return block_app_input_; }

void InputManager::set_block_app_input(bool block_app_input) {
  if (block_app_input_ != block_app_input) {
    if (block_app_input) {
      ReleasePressedKeys();
      SaveCursorState();
    } else {
      RestoreCursorState();
    }

    block_app_input_ = block_app_input;
  }
}

InputHook *InputManager::get_input_hook() { return &input_hook_; }

}  // namespace input
}  // namespace core
}  // namespace overlay