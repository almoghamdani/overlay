#include "input_hook.h"

#include <mutex>

#include "core.h"

namespace overlay {
namespace core {
namespace input {

bool InputHook::Hook() {
  HMODULE user32_module = LoadLibraryA("user32.dll");

  void *get_async_key_state_func =
      GetProcAddress(user32_module, "GetAsyncKeyState");
  void *get_key_state_func = GetProcAddress(user32_module, "GetKeyState");
  void *get_keyboard_state_func =
      GetProcAddress(user32_module, "GetKeyboardState");
  void *get_raw_input_data_func =
      GetProcAddress(user32_module, "GetRawInputData");
  void *get_raw_input_buffer_func =
      GetProcAddress(user32_module, "GetRawInputBuffer");
  void *show_cursor_func = GetProcAddress(user32_module, "ShowCursor");
  void *get_cursor_pos_func = GetProcAddress(user32_module, "GetCursorPos");
  void *set_cursor_pos_func = GetProcAddress(user32_module, "SetCursorPos");
  void *get_cursor_func = GetProcAddress(user32_module, "GetCursor");
  void *set_cursor_func = GetProcAddress(user32_module, "SetCursor");

  bool hooked = true;

  if (get_async_key_state_func) {
    hooked &= get_async_key_state_hook_.Install(
        get_async_key_state_func,
        static_cast<pFnGetAsyncKeyState>([](int virtual_key) {
          return Core::Get()
              ->get_input_manager()
              ->get_input_hook()
              ->GetAsyncKeyStateHook(virtual_key);
        }));
  }

  if (get_key_state_func) {
    hooked &= get_key_state_hook_.Install(
        get_key_state_func, static_cast<pFnGetKeyState>([](int virtual_key) {
          return Core::Get()
              ->get_input_manager()
              ->get_input_hook()
              ->GetKeyStateHook(virtual_key);
        }));
  }

  if (get_keyboard_state_func) {
    hooked &= get_keyboard_state_hook_.Install(
        get_keyboard_state_func,
        static_cast<pFnGetKeyboardState>([](PBYTE key_state_ptr) {
          return Core::Get()
              ->get_input_manager()
              ->get_input_hook()
              ->GetKeyboardStateHook(key_state_ptr);
        }));
  }

  if (get_raw_input_data_func) {
    hooked &= get_raw_input_data_hook_.Install(
        get_raw_input_data_func,
        static_cast<pFnGetRawInputData>([](HRAWINPUT raw_input_ptr,
                                           UINT command, LPVOID data_ptr,
                                           PUINT data_size, UINT header_size) {
          return Core::Get()
              ->get_input_manager()
              ->get_input_hook()
              ->GetRawInputDataHook(raw_input_ptr, command, data_ptr, data_size,
                                    header_size);
        }));
  }

  if (get_raw_input_buffer_func) {
    hooked &= get_raw_input_buffer_hook_.Install(
        get_raw_input_buffer_func,
        static_cast<pFnGetRawInputBuffer>(
            [](PRAWINPUT data_ptr, PUINT data_size, UINT header_size) {
              return Core::Get()
                  ->get_input_manager()
                  ->get_input_hook()
                  ->GetRawInputBufferHook(data_ptr, data_size, header_size);
            }));
  }

  if (show_cursor_func) {
    hooked &= show_cursor_hook_.Install(
        show_cursor_func, static_cast<pFnShowCursor>([](BOOL show) {
          return Core::Get()
              ->get_input_manager()
              ->get_input_hook()
              ->ShowCursorHook(show);
        }));
  }

  if (get_cursor_pos_func) {
    hooked &= get_cursor_pos_hook_.Install(
        get_cursor_pos_func,
        static_cast<pFnGetCursorPos>([](LPPOINT point_ptr) {
          return Core::Get()
              ->get_input_manager()
              ->get_input_hook()
              ->GetCursorPosHook(point_ptr);
        }));
  }

  if (set_cursor_pos_func) {
    hooked &= set_cursor_pos_hook_.Install(
        set_cursor_pos_func, static_cast<pFnSetCursorPos>([](int x, int y) {
          return Core::Get()
              ->get_input_manager()
              ->get_input_hook()
              ->SetCursorPosHook(x, y);
        }));
  }

  if (get_cursor_func) {
    hooked &= get_cursor_hook_.Install(get_cursor_func,
                                       static_cast<pFnGetCursor>([]() {
                                         return Core::Get()
                                             ->get_input_manager()
                                             ->get_input_hook()
                                             ->GetCursorHook();
                                       }));
  }

  if (set_cursor_func) {
    hooked &= set_cursor_hook_.Install(
        set_cursor_func, static_cast<pFnSetCursor>([](HCURSOR cursor_handle) {
          return Core::Get()
              ->get_input_manager()
              ->get_input_hook()
              ->SetCursorHook(cursor_handle);
        }));
  }

  return hooked;
}

SHORT InputHook::GetAsyncKeyStateHook(int virtual_key) {
  if (Core::Get()->get_input_manager()->get_block_app_input()) {
    return 0;
  }

  return get_async_key_state_hook_.get_trampoline().CallStdMethod<SHORT>(
      virtual_key);
}

SHORT InputHook::GetKeyStateHook(int virtual_key) {
  if (Core::Get()->get_input_manager()->get_block_app_input()) {
    return 0;
  }

  return get_key_state_hook_.get_trampoline().CallStdMethod<SHORT>(virtual_key);
}

BOOL InputHook::GetKeyboardStateHook(PBYTE key_state_ptr) {
  if (Core::Get()->get_input_manager()->get_block_app_input()) {
    memset(key_state_ptr, 0, 256);
    return TRUE;
  }

  return get_keyboard_state_hook_.get_trampoline().CallStdMethod<BOOL>(
      key_state_ptr);
}

UINT InputHook::GetRawInputDataHook(HRAWINPUT raw_input_ptr, UINT command,
                                    LPVOID data_ptr, PUINT data_size,
                                    UINT header_size) {
  if (Core::Get()->get_input_manager()->get_block_app_input()) {
    get_raw_input_data_hook_.get_trampoline().CallStdMethod<UINT>(
        raw_input_ptr, command, nullptr, data_size, header_size);

    return 0;
  }

  return get_raw_input_data_hook_.get_trampoline().CallStdMethod<UINT>(
      raw_input_ptr, command, data_ptr, data_size, header_size);
}

UINT InputHook::GetRawInputBufferHook(PRAWINPUT data_ptr, PUINT data_size,
                                      UINT header_size) {
  if (Core::Get()->get_input_manager()->get_block_app_input()) {
    get_raw_input_buffer_hook_.get_trampoline().CallStdMethod<UINT>(
        nullptr, data_size, header_size);

    return 0;
  }

  return get_raw_input_buffer_hook_.get_trampoline().CallStdMethod<UINT>(
      data_ptr, data_size, header_size);
}

int InputHook::ShowCursorHook(BOOL show) {
  if (Core::Get()->get_input_manager()->get_block_app_input()) {
    std::lock_guard cursor_state_lk(
        Core::Get()->get_input_manager()->cursor_state_mutex_);

    Core::Get()->get_input_manager()->cursor_state_.cursor_count +=
        show ? 1 : -1;

    return Core::Get()->get_input_manager()->cursor_state_.cursor_count;
  }

  return show_cursor_hook_.get_trampoline().CallStdMethod<int>(show);
}

BOOL InputHook::GetCursorPosHook(LPPOINT point_ptr) {
  if (Core::Get()->get_input_manager()->get_block_app_input()) {
    std::lock_guard cursor_state_lk(
        Core::Get()->get_input_manager()->cursor_state_mutex_);

    if (point_ptr) {
      *point_ptr = Core::Get()->get_input_manager()->cursor_state_.cursor_pos;
    }

    return true;
  }

  return get_cursor_pos_hook_.get_trampoline().CallStdMethod<BOOL>(point_ptr);
}

BOOL InputHook::SetCursorPosHook(int x, int y) {
  if (Core::Get()->get_input_manager()->get_block_app_input()) {
    std::lock_guard cursor_state_lk(
        Core::Get()->get_input_manager()->cursor_state_mutex_);

    Core::Get()->get_input_manager()->cursor_state_.cursor_pos = POINT{x, y};

    return true;
  }

  return set_cursor_pos_hook_.get_trampoline().CallStdMethod<BOOL>(x, y);
}

HCURSOR InputHook::GetCursorHook() {
  if (Core::Get()->get_input_manager()->get_block_app_input()) {
    std::lock_guard cursor_state_lk(
        Core::Get()->get_input_manager()->cursor_state_mutex_);

    return Core::Get()->get_input_manager()->cursor_state_.cursor_handle;
  }

  return get_cursor_hook_.get_trampoline().CallStdMethod<HCURSOR>();
}

HCURSOR InputHook::SetCursorHook(HCURSOR cursor_handle) {
  if (Core::Get()->get_input_manager()->get_block_app_input()) {
    std::lock_guard cursor_state_lk(
        Core::Get()->get_input_manager()->cursor_state_mutex_);

    HCURSOR old_cursor =
        Core::Get()->get_input_manager()->cursor_state_.cursor_handle;

    Core::Get()->get_input_manager()->cursor_state_.cursor_handle =
        cursor_handle;

    return old_cursor;
  }

  return set_cursor_hook_.get_trampoline().CallStdMethod<HCURSOR>(
      cursor_handle);
}

}  // namespace input
}  // namespace core
}  // namespace overlay