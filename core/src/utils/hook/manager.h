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
#include <vector>

#include "address.h"
#include "section.h"

namespace overlay {
namespace hook {
class manager {
   public:
    static void set_exe_memory(void *exe_memory);
    static void *get_exe_memory();

    static std::vector<section> get_sections();

    static bool install_hook(std::string hook_name, void *src, void *dst);
    static bool install_winapi_hook(std::string hook_name, HMODULE module, const char *proc_name,
                                    void *dst);
    static std::shared_ptr<subhook::Hook> get_hook_ptr(std::string hook_name);

    template <int hook_idx, typename R, typename... Args>
    static typename std::enable_if<!std::is_same<R, void>::value, bool>::type install_logger_hook(
        void *src) {
        return install_hook(
            std::to_string(hook_idx), src, +[](Args... args) -> R {
                subhook::ScopedHookRemove h(get_hook_ptr(std::to_string(hook_idx)).get());

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
                    ((R(*)(Args...))get_hook_ptr(std::to_string(hook_idx))->GetSrc())(args...);

                // Print the params and the return value
                format = "FUNCTION {} " +
                         (args_size ? "PARAMS: " + string_multiply("{} ", args_size) : "") +
                         "RETURN: {}";
                // spdlog::get("Function Logger")->info(format, hook_idx, args..., ret_value);

                return ret_value;
            });
    }

    template <int hook_idx, typename R, typename... Args>
    static typename std::enable_if<std::is_same<R, void>::value, bool>::type install_logger_hook(
        void *src) {
        return install_hook(
            std::to_string(hook_idx), src, +[](Args... args) {
                subhook::ScopedHookRemove h(get_hook_ptr(std::to_string(hook_idx)).get());

                size_t args_size = sizeof...(args);
                std::string format;

                auto string_multiply = [](std::string str, int n) -> std::string {
                    std::stringstream out;
                    while (n--) out << str;
                    return out.str();
                };

                // Print params
                format = "FUNCTION {} " +
                         (args_size ? "PARAMS: " + string_multiply("{} ", args_size) : "");
                // spdlog::get("Function Logger")->info(format, hook_idx, args...);

                // Call the original function
                ((void (*)(Args...))get_hook_ptr(std::to_string(hook_idx))->GetSrc())(args...);
            });
    }

    static bool remove_hook(std::string hook_name);
    static address get_trampoline(std::string hook_name);

   private:
    inline static std::unordered_map<std::string, std::shared_ptr<subhook::Hook>> *_hooks =
        new std::unordered_map<std::string, std::shared_ptr<subhook::Hook>>();
    inline static void *exe_memory = 0;
};
};  // namespace hook
};  // namespace overlay