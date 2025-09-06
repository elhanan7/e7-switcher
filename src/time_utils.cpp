#include "e7-switcher/time_utils.h"
#include <chrono>

namespace e7_switcher {

int32_t java_bug_seconds_from_now() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return static_cast<int32_t>(ms / 1000);
}

uint32_t now_unix() {
    auto now = std::chrono::system_clock::now();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
}

} // namespace e7_switcher
