#include "manager.h"

#include <algorithm>

namespace overlay {
namespace utils {
namespace hook {

bool Manager::InstallHook(std::string hook_name, void *src, void *dst) {
  bool success = false;
  std::shared_ptr<subhook::Hook> hook = std::make_shared<subhook::Hook>();

  // Try to install the hook
#ifdef _WIN64
  success = hook->Install(src, dst, subhook::HookFlag64BitOffset);
#else
  success = hook->Install(src, dst);
#endif

  // Add the hook to the map of hooks
  {
    std::lock_guard lk(hooks_mutex_);
    hooks_[hook_name] = hook;
  }

  return success;
}

bool Manager::InstallWinapiHook(std::string hook_name, HMODULE module,
                                const char *proc_name, void *dst) {
  void *addr = GetProcAddress(module, proc_name);

  return InstallHook(hook_name, addr, dst);
}

std::shared_ptr<subhook::Hook> Manager::GetHookPtr(std::string hook_name) {
  std::lock_guard lk(hooks_mutex_);

  try {
    return hooks_.at(hook_name);
  } catch (...) {
    throw std::runtime_error("The hook " + hook_name + " wasn't found");
  }
}

bool Manager::RemoveHook(std::string hook_name) {
  std::shared_ptr<subhook::Hook> hook;

  std::lock_guard lk(hooks_mutex_);

  try {
    hook = hooks_.at(hook_name);

    hooks_.erase(hook_name);
  } catch (...) {
    throw std::runtime_error("The hook " + hook_name + " wasn't found");
  }

  return hook->Remove();
}

Address Manager::GetTrampoline(std::string hook_name) {
  std::shared_ptr<subhook::Hook> hook;

  std::lock_guard lk(hooks_mutex_);

  try {
    hook = hooks_.at(hook_name);
  } catch (...) {
    throw std::runtime_error("The hook " + hook_name + " wasn't found");
  }

  return Address((uintptr_t)hook->GetTrampoline());
}

}  // namespace hook
}  // namespace utils
}  // namespace overlay