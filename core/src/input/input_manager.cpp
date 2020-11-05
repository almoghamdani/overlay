#include "input_manager.h"

#include <commctrl.h>

#include <loguru/loguru.hpp>

#include "core.h"
#include "events.pb.h"
#include "graphics/window.h"
#include "utils/rect.h"

#define KEY_UP_PASSTHROUGH_LPARAM ((LPARAM)0xA1B3C5D7)
#define SUBCLASS_WINDOW_MESSAGE (WM_USER + 0xBC)

namespace overlay {
namespace core {
namespace input {

LRESULT CALLBACK WindowGetMsgHook(_In_ int code, _In_ WPARAM word_param,
                                  _In_ LPARAM long_param) {
  return Core::Get()->get_input_manager()->WindowMsgHook(code, word_param,
                                                         long_param);
}

LRESULT CALLBACK WindowSubclassProc(_In_ HWND window, _In_ UINT message,
                                    _In_ WPARAM word_param,
                                    _In_ LPARAM long_param,
                                    _In_ UINT_PTR subclass_id,
                                    _In_ InputManager *input_manager) {
  return input_manager->WindowSubclassProc(window, message, word_param,
                                           long_param);
}

InputManager::InputManager()
    : block_app_input_(false),
      block_app_input_cursor_(LoadCursor(NULL, IDC_ARROW)),
      window_msg_hook_(NULL),
      resizing_moving_(false) {}

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

void InputManager::SetCursorCounter(int counter) {
  int cursor_count =
      input_hook_.show_cursor_hook_.get_trampoline().CallStdMethod<int>(false);
  bool show_cursor = cursor_count < counter;

  while (cursor_count != counter) {
    cursor_count =
        input_hook_.show_cursor_hook_.get_trampoline().CallStdMethod<int>(
            show_cursor);
  }
}

void InputManager::SaveCursorState() {
  std::lock_guard app_cursor_state_lk(app_cursor_state_mutex_);

  app_cursor_state_.cursor_count =
      input_hook_.show_cursor_hook_.get_trampoline().CallStdMethod<int>(true) -
      1;
  input_hook_.show_cursor_hook_.get_trampoline().CallStdMethod<int>(false);

  app_cursor_state_.cursor_handle =
      input_hook_.get_cursor_hook_.get_trampoline().CallStdMethod<HCURSOR>();

  input_hook_.get_cursor_pos_hook_.get_trampoline().CallStdMethod<BOOL>(
      &app_cursor_state_.cursor_pos);

  DLOG_F(INFO,
         "Saved cursor state - Count: %d, Position: (%d, %d), Handle: 0x%x.",
         app_cursor_state_.cursor_count, app_cursor_state_.cursor_pos.x,
         app_cursor_state_.cursor_pos.y, app_cursor_state_.cursor_handle);
}

void InputManager::RestoreCursorState() {
  std::lock_guard app_cursor_state_lk(app_cursor_state_mutex_);

  // Restore cursor count
  SetCursorCounter(app_cursor_state_.cursor_count);

  input_hook_.set_cursor_hook_.get_trampoline().CallStdMethod<HCURSOR>(
      app_cursor_state_.cursor_handle);
  input_hook_.set_cursor_pos_hook_.get_trampoline().CallStdMethod<BOOL>(
      app_cursor_state_.cursor_pos.x, app_cursor_state_.cursor_pos.y);

  DLOG_F(INFO,
         "Restored cursor state - Count: %d, Position: (%d, %d), Handle: 0x%x.",
         app_cursor_state_.cursor_count, app_cursor_state_.cursor_pos.x,
         app_cursor_state_.cursor_pos.y, app_cursor_state_.cursor_handle);
}

void InputManager::HandleKeyboardInput(UINT message, uint32_t param) {
  EventResponse event;
  EventResponse::WindowEvent *window_event = event.mutable_windowevent();
  EventResponse::WindowEvent::KeyboardInputEvent *input_event =
      window_event->mutable_keyboardinputevent();

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
      return;
  }

  Core::Get()
      ->get_graphics_manager()
      ->get_window_manager()
      ->SendWindowEventToFocusedWindow(event);
}

void InputManager::HandleMouseInput(UINT message, POINT point,
                                    WPARAM word_param) {
  EventResponse event;
  EventResponse::WindowEvent *window_event = event.mutable_windowevent();
  EventResponse::WindowEvent::MouseInputEvent *input_event =
      window_event->mutable_mouseinputevent();

  switch (message) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
      input_event->set_type(message == WM_LBUTTONDBLCLK
                                ? EventResponse::WindowEvent::MouseInputEvent::
                                      MOUSE_BUTTON_DOUBLE_CLICK
                                : message == WM_LBUTTONDOWN
                                      ? EventResponse::WindowEvent::
                                            MouseInputEvent::MOUSE_BUTTON_DOWN
                                      : EventResponse::WindowEvent::
                                            MouseInputEvent::MOUSE_BUTTON_UP);
      input_event->set_button(
          EventResponse::WindowEvent::MouseInputEvent::LEFT_BUTTON);
      break;

    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
      input_event->set_type(message == WM_MBUTTONDBLCLK
                                ? EventResponse::WindowEvent::MouseInputEvent::
                                      MOUSE_BUTTON_DOUBLE_CLICK
                                : message == WM_MBUTTONDOWN
                                      ? EventResponse::WindowEvent::
                                            MouseInputEvent::MOUSE_BUTTON_DOWN
                                      : EventResponse::WindowEvent::
                                            MouseInputEvent::MOUSE_BUTTON_UP);
      input_event->set_button(
          EventResponse::WindowEvent::MouseInputEvent::MIDDLE_BUTTON);
      break;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
      input_event->set_type(message == WM_RBUTTONDBLCLK
                                ? EventResponse::WindowEvent::MouseInputEvent::
                                      MOUSE_BUTTON_DOUBLE_CLICK
                                : message == WM_RBUTTONDOWN
                                      ? EventResponse::WindowEvent::
                                            MouseInputEvent::MOUSE_BUTTON_DOWN
                                      : EventResponse::WindowEvent::
                                            MouseInputEvent::MOUSE_BUTTON_UP);
      input_event->set_button(
          EventResponse::WindowEvent::MouseInputEvent::RIGHT_BUTTON);
      break;

    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_XBUTTONDBLCLK:
      input_event->set_type(message == WM_XBUTTONDBLCLK
                                ? EventResponse::WindowEvent::MouseInputEvent::
                                      MOUSE_BUTTON_DOUBLE_CLICK
                                : message == WM_XBUTTONDOWN
                                      ? EventResponse::WindowEvent::
                                            MouseInputEvent::MOUSE_BUTTON_DOWN
                                      : EventResponse::WindowEvent::
                                            MouseInputEvent::MOUSE_BUTTON_UP);
      input_event->set_button(
          GET_XBUTTON_WPARAM(word_param) == XBUTTON1
              ? EventResponse::WindowEvent::MouseInputEvent::X_BUTTON_1
              : EventResponse::WindowEvent::MouseInputEvent::X_BUTTON_2);
      break;

    case WM_MOUSEMOVE:
      input_event->set_type(
          EventResponse::WindowEvent::MouseInputEvent::MOUSE_MOVE);
      break;

    case WM_MOUSEWHEEL:
      input_event->set_type(
          EventResponse::WindowEvent::MouseInputEvent::MOUSE_VERTICAL_WHEEL);
      input_event->set_wheeldelta(GET_WHEEL_DELTA_WPARAM(word_param));
      break;

    case WM_MOUSEHWHEEL:
      input_event->set_type(
          EventResponse::WindowEvent::MouseInputEvent::MOUSE_HORIZONTAL_WHEEL);
      input_event->set_wheeldelta(GET_WHEEL_DELTA_WPARAM(word_param));
      break;

    default:
      return;
  }

  Core::Get()->get_graphics_manager()->get_window_manager()->HandleMouseEvent(
      event, point);
}

bool InputManager::HookWindow(HWND window) {
  DWORD thread = GetWindowThreadProcessId(window, NULL);

  window_msg_hook_ =
      SetWindowsHookExW(WH_GETMESSAGE, WindowGetMsgHook, NULL, thread);

  return GetClientRect(window, &window_client_area_) &&
         PostMessage(window, SUBCLASS_WINDOW_MESSAGE, NULL, NULL) &&
         window_msg_hook_ != NULL;
}

LRESULT InputManager::WindowMsgHook(_In_ int code, _In_ WPARAM word_param,
                                    _In_ LPARAM long_param) {
  MSG *message = NULL;

  if (code >= 0) {
    message = (MSG *)long_param;

    // Check that the message is for the correct window
    if (message->hwnd == Core::Get()->get_graphics_window() &&
        word_param == PM_REMOVE) {
      if (message->message == SUBCLASS_WINDOW_MESSAGE) {
        if (!SetWindowSubclass(message->hwnd,
                               (SUBCLASSPROC)input::WindowSubclassProc,
                               (UINT_PTR)this, (DWORD_PTR)this)) {
          LOG_F(ERROR, "Unable to install window subclass!");
        }
        message->message = WM_NULL;
        return 0;
      }

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
          HandleKeyboardInput(message->message, (uint32_t)message->wParam);

          // Block application input
          message->message = WM_NULL;
        }
      }

      if (message->message >= WM_MOUSEFIRST &&
          message->message <= WM_MOUSELAST) {
        POINTS point_short = MAKEPOINTS(message->lParam);
        POINT point = {point_short.x, point_short.y};

        // Convert screen relative point to client area relative point when
        // needed
        if (message->message == WM_MOUSEWHEEL ||
            message->message == WM_MOUSEHWHEEL) {
          ScreenToClient(message->hwnd, &point);
        }

        if (block_app_input_ && !resizing_moving_ &&
            utils::Rect::PointInRect(point, window_client_area_)) {
          // Handle input
          HandleMouseInput(message->message, point, message->wParam);

          // Block application input
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

LRESULT InputManager::WindowSubclassProc(_In_ HWND window, _In_ UINT message,
                                         _In_ WPARAM word_param,
                                         _In_ LPARAM long_param) {
  if (window == Core::Get()->get_graphics_window()) {
    switch (message) {
      case WM_ENTERSIZEMOVE:
        resizing_moving_ = true;
        break;

      case WM_EXITSIZEMOVE:
        resizing_moving_ = false;
        break;

      case WM_SIZE:
        GetClientRect(window, &window_client_area_);
        break;

      case WM_SETCURSOR:
        // Set the cursor if app input is blocked
        if (block_app_input_ && LOWORD(long_param) == HTCLIENT) {
          input_hook_.set_cursor_hook_.get_trampoline().Call<HCURSOR, HCURSOR>(
              block_app_input_cursor_);
          return 0;
        }
        break;
    }
  }

  return DefSubclassProc(window, message, word_param, long_param);
}

bool InputManager::get_block_app_input() const { return block_app_input_; }

void InputManager::set_block_app_input(bool block_app_input) {
  if (block_app_input_ != block_app_input) {
    if (block_app_input) {
      // Send WM_KEYUP to pressed keys and save the app's cursor state
      ReleasePressedKeys();
      SaveCursorState();
    } else {
      RestoreCursorState();
    }

    block_app_input_ = block_app_input;

    // Show cursor
    SetCursorCounter(0);

    // Send WM_SETCURSOR to the window proc to set the cursor
    SendMessage(Core::Get()->get_graphics_window(), WM_SETCURSOR,
                (WPARAM)Core::Get()->get_graphics_window(),
                MAKELPARAM(HTCLIENT, 0));
  }
}

void InputManager::set_block_app_input_cursor(HCURSOR cursor) {
  if (block_app_input_cursor_ != cursor) {
    block_app_input_cursor_ = cursor;

    if (block_app_input_) {
      // Send WM_SETCURSOR to the window proc to set the cursor
      SendMessage(Core::Get()->get_graphics_window(), WM_SETCURSOR,
                  (WPARAM)Core::Get()->get_graphics_window(),
                  MAKELPARAM(HTCLIENT, 0));
    }
  }
}

InputHook *InputManager::get_input_hook() { return &input_hook_; }

}  // namespace input
}  // namespace core
}  // namespace overlay