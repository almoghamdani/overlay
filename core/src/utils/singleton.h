#pragma once

namespace overlay {
namespace utils {
template <typename T>
class singleton {
   public:
    static T *get() {
        if (!_instance) {
            _instance = new T;
        }

        return _instance;
    }

   private:
    inline static T *_instance = nullptr;
};
};  // namespace utils
};  // namespace overlay