#pragma once

#include "address.h"
#include "manager.h"

namespace overlay {
namespace hook {
class scoped_hook_removal {
   public:
    scoped_hook_removal(std::string hook_name)
        : _hook(manager::get_hook_ptr(hook_name)), _hook_remove(_hook.get()) {}

    address get() { return address((uint64_t)_hook->GetSrc()); };

   private:
    std::shared_ptr<subhook::Hook> _hook;
    subhook::ScopedHookRemove _hook_remove;
};
};  // namespace hook
};  // namespace overlay