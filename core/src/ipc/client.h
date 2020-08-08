#pragma once
#include <windows.h>

namespace overlay {
namespace core {
namespace ipc {

struct Client {
  DWORD process_id;
};

}  // namespace ipc
}  // namespace core
}  // namespace overlay