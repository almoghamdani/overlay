#pragma once

namespace overlay {
namespace utils {

template <typename T>
class Singleton {
 public:
  static T *Get() {
    if (!instance_) {
      instance_ = new T;
    }

    return instance_;
  }

 private:
  inline static T *instance_ = nullptr;
};

}  // namespace utils
}  // namespace overlay