#ifndef OVERLAY_HELPER_H
#define OVERLAY_HELPER_H
#include <Windows.h>

#ifdef OVERLAY_HELPER
#define HELPER_EXPORT __declspec(dllexport)
#else
#define HELPER_EXPORT __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

HELPER_EXPORT bool OverlayInjectToProcess(DWORD pid);

#ifdef __cplusplus
}
#endif

#endif