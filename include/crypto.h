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

std::vector<uint8_t> decrypt_hex_ecb_pkcs7(const std::vector<uint8_t>& ciphertext, const std::string& key);
std::vector<uint8_t> encrypt_to_hex_ecb_pkcs7(const std::vector<uint8_t>& plaintext, const std::string& key);

} // namespace e7_switcher
