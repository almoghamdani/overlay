#include "utils/singleton.h"

#include <Windows.h>
#include <thread>

#include "graphics/dxgi_hook.h"

namespace overlay {
namespace core {
class core : public utils::singleton<core> {
   public:
    void start();

    void setInjectWindow(HWND window);
    HWND getInjectWindow() const;

    void setGraphicsWindow(HWND window);
    HWND getGraphicsWindow() const;

    graphics::dxgi_hook *getDxgiHook();

   private:
    std::thread _mainThread;

    HWND _injectWindow;
    HWND _graphicsWindow;

    graphics::dxgi_hook _dxgiHook;

    void main_thread();
};
};  // namespace core
};  // namespace overlay