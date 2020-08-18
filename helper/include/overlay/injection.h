#ifndef OVERLAY_INJECTION_H
#define OVERLAY_INJECTION_H
#include <overlay/export.h>
#include <windows.h>

namespace overlay {
namespace helper {

HELPER_EXPORT void InjectCoreToProcess(DWORD pid);

}  // namespace helper
}  // namespace overlay

#endif