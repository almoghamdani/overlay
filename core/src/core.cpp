#include "core.h"

void overlay::core::core::start() {
    // Start core main thread
    _mainThread = std::thread(&core::main_thread, this);
}

void overlay::core::core::main_thread() {
    // Hook DXGI api (DirectX 10 - 12)
    _dxgiHook.hook();
}