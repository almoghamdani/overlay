#pragma once
#include <Windows.h>

#include "utils/hook/hook.h"

typedef SHORT(WINAPI *pFnGetAsyncKeyState)(int virtual_key);
typedef SHORT(WINAPI *pFnGetKeyState)(int virtual_key);
typedef BOOL(WINAPI *pFnGetKeyboardState)(PBYTE key_state_ptr);
typedef UINT(WINAPI *pFnGetRawInputData)(HRAWINPUT raw_input_ptr, UINT command,
                                         LPVOID data_ptr, PUINT data_size,
                                         UINT header_size);
typedef UINT(WINAPI *pFnGetRawInputBuffer)(PRAWINPUT data_ptr, PUINT data_size,
                                           UINT header_size);
typedef int(WINAPI *pFnShowCursor)(BOOL show);
typedef BOOL(WINAPI *pFnGetCursorPos)(LPPOINT point_ptr);
typedef BOOL(WINAPI *pFnSetCursorPos)(int x, int y);
typedef HCURSOR(WINAPI *pFnGetCursor)();
typedef HCURSOR(WINAPI *pFnSetCursor)(HCURSOR cursor_handle);

namespace overlay {
namespace core {
namespace input {

class InputHook {
 public:
  bool Hook();

  SHORT GetAsyncKeyStateHook(int virtual_key);
  SHORT GetKeyStateHook(int virtual_key);
  BOOL GetKeyboardStateHook(PBYTE key_state_ptr);
  UINT GetRawInputDataHook(HRAWINPUT raw_input_ptr, UINT command,
                           LPVOID data_ptr, PUINT data_size, UINT header_size);
  UINT GetRawInputBufferHook(PRAWINPUT data_ptr, PUINT data_size,
                             UINT header_size);
  int ShowCursorHook(BOOL show);
  BOOL GetCursorPosHook(LPPOINT point_ptr);
  BOOL SetCursorPosHook(int x, int y);
  HCURSOR GetCursorHook();
  HCURSOR SetCursorHook(HCURSOR cursor_handle);

 private:
  utils::hook::Hook get_async_key_state_hook_, get_key_state_hook_,
      get_keyboard_state_hook_, get_raw_input_data_hook_,
      get_raw_input_buffer_hook_, show_cursor_hook_, get_cursor_pos_hook_,
      set_cursor_pos_hook_, set_cursor_hook_, get_cursor_hook_;

  friend class InputManager;
};

}  // namespace input
}  // namespace core
}  // namespace overlay