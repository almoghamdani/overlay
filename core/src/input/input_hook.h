#pragma once
#include <Windows.h>

#include "utils/hook/hook.h"

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

  int ShowCursorHook(BOOL show);
  BOOL GetCursorPosHook(LPPOINT point_ptr);
  BOOL SetCursorPosHook(int x, int y);
  HCURSOR GetCursorHook();
  HCURSOR SetCursorHook(HCURSOR cursor_handle);

 private:
  utils::hook::Hook show_cursor_hook_, get_cursor_pos_hook_,
      set_cursor_pos_hook_, set_cursor_hook_, get_cursor_hook_;

  friend class InputManager;
};

}  // namespace input
}  // namespace core
}  // namespace overlay