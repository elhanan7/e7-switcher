#include "e7-switcher/crc.h"
#include <vector>

namespace e7_switcher {

uint16_t crc_hqx(const uint8_t *data, size_t len, uint16_t crc) {
    while (len--) {
        crc ^= (uint16_t)(*data++) << 8;
        for (int i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

std::vector<uint8_t> get_complete_legal_crc(const std::vector<uint8_t>& payload, const std::vector<uint8_t>& key) {
    // 1) first CRC over payload
    uint16_t crc_a = crc_hqx(payload.data(), payload.size(), 0x1021);

    // 2) build the 34-byte block
    std::vector<uint8_t> block(34, 0);
    block[0] = crc_a & 0xFF;
    block[1] = (crc_a >> 8) & 0xFF;

    if (key.empty()) {
        // block[2:34] are already 0
    } else {
        for (size_t i = 0; i < key.size() && i < 32; ++i) {
            block[i + 2] = key[i];
        }
    }

    // 3) second CRC over block
    uint16_t crc_b = crc_hqx(block.data(), block.size(), 0x1021);

    // 4) return 4 bytes: first crcA, then crcB (same endianness)
    std::vector<uint8_t> out(4);
    out[0] = crc_a & 0xFF;
    out[1] = (crc_a >> 8) & 0xFF;
    out[2] = crc_b & 0xFF;
    out[3] = (crc_b >> 8) & 0xFF;
    return out;
}

} // namespace e7_switcher
