#include "utils/singleton.h"

#include <thread>

#include "graphics/dxgi_hook.h"

namespace overlay {
namespace core {
class core : public utils::singleton<core> {
   public:
    void start();

    void main_thread();

   private:
    std::thread _mainThread;

    graphics::dxgi_hook _dxgiHook;
};
};  // namespace core
};  // namespace overlay