#include <Windows.h>

#include "core.h"

extern "C" __declspec(dllexport) LRESULT CALLBACK
    msg_hook(int code, WPARAM wParam, LPARAM lParam) {
  static bool injectWindowSet = false;

  // If the inject window was not set yet, set it
  if (!injectWindowSet) {
    overlay::core::Core::Get()->set_inject_window(((MSG *)lParam)->hwnd);
    injectWindowSet = true;
  }

  return CallNextHookEx(0, code, wParam, lParam);
}