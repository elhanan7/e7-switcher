#pragma once

#include <vector>
#include <string>

#ifdef ESP_PLATFORM
#include "mbedtls/aes.h"
#else
#include <openssl/evp.h>
#include <openssl/aes.h>
#endif

namespace e7_switcher {

std::vector<uint8_t> aes_decrypt(const std::vector<uint8_t>& ciphertext, const std::string& key);
std::vector<uint8_t> aes_encrypt(const std::vector<uint8_t>& plaintext, const std::string& key);

} // namespace e7_switcher
