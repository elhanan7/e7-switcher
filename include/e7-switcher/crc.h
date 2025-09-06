#pragma once

#include <cstdint>
#include <vector>

namespace e7_switcher {

std::vector<uint8_t> get_complete_legal_crc(const std::vector<uint8_t>& payload, const std::vector<uint8_t>& key);

} // namespace e7_switcher
