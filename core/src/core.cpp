#include "core.h"

#include <loguru.hpp>

void overlay::core::core::start() {
    // Start core main thread
    _mainThread = std::thread(&core::main_thread, this);
}

void overlay::core::core::main_thread() {
    // Hook DXGI api (DirectX 10 - 12)
    _dxgiHook.hook();
}

void overlay::core::core::setInjectWindow(HWND window) {
    LOG_F(INFO, "Application's main thread window is 0x%x.");
    _injectWindow = window;
}

HWND overlay::core::core::getInjectWindow() const { return _injectWindow; }

void overlay::core::core::setGraphicsWindow(HWND window) {
    LOG_F(INFO, "Setting graphics window to 0x%x.");
    _graphicsWindow = window;
}

HWND overlay::core::core::getGraphicsWindow() const { return _graphicsWindow; }

overlay::core::graphics::dxgi_hook *overlay::core::core::getDxgiHook() { return &_dxgiHook; }