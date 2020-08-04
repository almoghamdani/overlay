#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <subhook.h>

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include "address.h"

namespace overlay {
namespace utils {
namespace hook {

class Manager {
 public:
  static bool InstallHook(std::string hook_name, void *src, void *dst);
  static bool InstallWinapiHook(std::string hook_name, HMODULE module,
                                const char *proc_name, void *dst);
  static std::shared_ptr<subhook::Hook> GetHookPtr(std::string hook_name);

  template <int hook_idx, typename R, typename... Args>
  static typename std::enable_if<!std::is_same<R, void>::value, bool>::type
  InstallLoggerHook(void *src) {
    return InstallHook(
        std::to_string(hook_idx), src, +[](Args... args) -> R {
          subhook::ScopedHookRemove h(
              GetHookPtr(std::to_string(hook_idx)).get());

          size_t args_size = sizeof...(args);
          std::string format;

          // Function used to multiply a string
          auto string_multiply = [](std::string str, int n) -> std::string {
            std::stringstream out;
            while (n--) out << str;
            return out.str();
          };

          // Call the original function and get the return value
          R ret_value =
              ((R(*)(Args...))GetHookPtr(std::to_string(hook_idx))->GetSrc())(
                  args...);

          // Print the params and the return value
          format = "FUNCTION {} " +
                   (args_size ? "PARAMS: " + string_multiply("{} ", args_size)
                              : "") +
                   "RETURN: {}";
          // spdlog::get("Function Logger")->info(format, hook_idx, args...,
          // ret_value);

          return ret_value;
        });
  }

  template <int hook_idx, typename R, typename... Args>
  static typename std::enable_if<std::is_same<R, void>::value, bool>::type
  InstallLoggerHook(void *src) {
    return InstallHook(
        std::to_string(hook_idx), src, +[](Args... args) {
          subhook::ScopedHookRemove h(
              GetHookPtr(std::to_string(hook_idx)).get());

          size_t args_size = sizeof...(args);
          std::string format;

          auto string_multiply = [](std::string str, int n) -> std::string {
            std::stringstream out;
            while (n--) out << str;
            return out.str();
          };

          // Print params
          format =
              "FUNCTION {} " +
              (args_size ? "PARAMS: " + string_multiply("{} ", args_size) : "");
          // spdlog::get("Function Logger")->info(format, hook_idx, args...);

          // Call the original function
          ((void (*)(Args...))GetHookPtr(std::to_string(hook_idx))->GetSrc())(
              args...);
        });
  }

  static bool RemoveHook(std::string hook_name);
  static Address GetTrampoline(std::string hook_name);

 private:
  inline static std::unordered_map<std::string, std::shared_ptr<subhook::Hook>>
      *hooks_ =
          new std::unordered_map<std::string, std::shared_ptr<subhook::Hook>>();
};

}  // namespace hook
}  // namespace utils
}  // namespace overlay