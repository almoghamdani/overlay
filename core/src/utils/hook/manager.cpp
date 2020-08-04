#include "manager.h"

#include <algorithm>

namespace overlay {
namespace utils {
namespace hook {

bool Manager::InstallHook(std::string hook_name, void *src, void *dst) {
  bool success = false;
  std::shared_ptr<subhook::Hook> hook = std::make_shared<subhook::Hook>();

  // Try to install the hook
  success = hook->Install(src, dst, subhook::HookFlag64BitOffset);

  // Add the hook to the map of hooks
  hooks_->insert({hook_name, hook});

  return success;
}

bool Manager::InstallWinapiHook(std::string hook_name, HMODULE module,
                                const char *proc_name, void *dst) {
  void *addr = GetProcAddress(module, proc_name);

  return InstallHook(hook_name, addr, dst);
}

std::shared_ptr<subhook::Hook> Manager::GetHookPtr(std::string hook_name) {
  try {
    return hooks_->find(hook_name)->second;
  } catch (...) {
    throw std::runtime_error("The hook " + hook_name + " wasn't found");
  }
}

bool Manager::RemoveHook(std::string hook_name) {
  std::shared_ptr<subhook::Hook> hook;

  try {
    hook = hooks_->find(hook_name)->second;

    hooks_->erase(hook_name);
  } catch (...) {
    throw std::runtime_error("The hook " + hook_name + " wasn't found");
  }

  return hook->Remove();
}

Address Manager::GetTrampoline(std::string hook_name) {
  std::shared_ptr<subhook::Hook> hook;

  try {
    hook = hooks_->find(hook_name)->second;
  } catch (...) {
    throw std::runtime_error("The hook " + hook_name + " wasn't found");
  }

  return Address((uintptr_t)hook->GetTrampoline());
}

}  // namespace hook
}  // namespace utils
}  // namespace overlay