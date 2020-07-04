#include <Windows.h>

extern "C" __declspec(dllexport) LRESULT CALLBACK msg_hook(int code,
														   WPARAM wParam,
														   LPARAM lParam)
{
	return CallNextHookEx(0, code, wParam, lParam);
}