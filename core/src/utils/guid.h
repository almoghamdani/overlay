#pragma once
#include <guiddef.h>
#include <rpc.h>

#include <utility>

// Simple hash for GUID
namespace std {

template <>
struct hash<GUID> {
  size_t operator()(const GUID &Value) const {
    RPC_STATUS status = RPC_S_OK;
    return UuidHash(&const_cast<GUID &>(Value), &status);
  }
};

}  // namespace std

namespace overlay {
namespace utils {

class Guid {
 public:
  static GUID GenerateGuid();
};

}  // namespace utils
}  // namespace overlay