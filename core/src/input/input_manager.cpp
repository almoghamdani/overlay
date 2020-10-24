#include "input_manager.h"

#include <loguru.hpp>

#include "core.h"

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
    LOG_F(ERROR, "Unable to hook input WinAPI functions!");
    return false;
  } else {
    LOG_F(INFO, "Hooked input WinAPI functions successfully!");
  }

  return true;
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

  LOG_F(INFO,
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

  LOG_F(INFO,
        "Restored cursor state - Count: %d, Position: (%d, %d), Handle: 0x%x.",
        cursor_state_.cursor_count, cursor_state_.cursor_pos.x,
        cursor_state_.cursor_pos.y, cursor_state_.cursor_handle);
}

bool InputManager::HookWindow(HWND window) {
  DWORD thread = GetWindowThreadProcessId(window, NULL);

  window_msg_hook_ =
      SetWindowsHookExA(WH_GETMESSAGE, WindowGetMsgHook, NULL, thread);

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
        if (block_app_input_) {
          message->message = WM_NULL;
        }
      }

      if (message->message >= WM_MOUSEFIRST && message->message <= WM_MOUSELAST) {
        if (block_app_input_) {
          message->message = WM_NULL;
        }
      }
    }
  }

  return CallNextHookEx(window_msg_hook_, code, word_param, long_param);
}

bool InputManager::get_block_app_input() const { return block_app_input_; }

void InputManager::set_block_app_input(bool block_app_input) {
  if (block_app_input_ != block_app_input) {
    if (block_app_input) {
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