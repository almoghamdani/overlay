#pragma once
#include <Windows.h>

#include <atomic>
#include <mutex>

#include "input_hook.h"

namespace overlay {
namespace core {
namespace input {

struct CursorState {
  int cursor_count;
  POINT cursor_pos;
  HCURSOR cursor_handle;
};

class InputManager {
 public:
  InputManager();

  bool Hook();
  bool HookWindow(HWND window);

  bool get_block_app_input() const;
  void set_block_app_input(bool block_app_input);

  InputHook *get_input_hook();

 private:
  std::atomic<bool> block_app_input_;

  CursorState cursor_state_;
  std::mutex cursor_state_mutex_;

  InputHook input_hook_;
  HHOOK window_msg_hook_, window_proc_hook_;

  bool resizing_moving_;
  RECT window_client_area_;

  void ReleasePressedKeys();
  uint16_t VirtualKeyToScanCode(uint8_t virtual_key);

  void SaveCursorState();
  void RestoreCursorState();

  void HandleInput(UINT message, uint32_t param);

  LRESULT WindowMsgHook(_In_ int code, _In_ WPARAM word_param,
                        _In_ LPARAM long_param);
  LRESULT WindowProcHook(_In_ int code, _In_ WPARAM word_param,
                         _In_ LPARAM long_param);

  friend LRESULT CALLBACK WindowGetMsgHook(_In_ int code,
                                           _In_ WPARAM word_param,
                                           _In_ LPARAM long_param);
  friend LRESULT CALLBACK CallWindowProcHook(_In_ int code,
                                             _In_ WPARAM word_param,
                                             _In_ LPARAM long_param);

  friend class InputHook;
};

}  // namespace input
}  // namespace core
}  // namespace overlay