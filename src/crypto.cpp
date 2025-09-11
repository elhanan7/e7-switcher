#include "e7-switcher/crypto.h"
#include <stdexcept>

#ifdef ESP_PLATFORM

namespace e7_switcher {

std::vector<uint8_t> aes_decrypt(const std::vector<uint8_t>& ciphertext, const std::string& key) {
    mbedtls_aes_context aes;
    std::vector<uint8_t> plaintext(ciphertext.size());

    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, (const unsigned char*)key.c_str(), key.length() * 8);

    for (size_t i = 0; i < ciphertext.size(); i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, ciphertext.data() + i, plaintext.data() + i);
    }

    mbedtls_aes_free(&aes);

    // PKCS7 padding removal
    size_t padding = plaintext.back();
    if (padding > 0 && padding <= 16) {
        plaintext.resize(plaintext.size() - padding);
    }

    return plaintext;
}

std::vector<uint8_t> aes_encrypt(const std::vector<uint8_t>& plaintext, const std::string& key) {
    mbedtls_aes_context aes;
    std::vector<uint8_t> ciphertext;
    
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char*)key.c_str(), key.length() * 8);

    // PKCS7 padding
    size_t padding = 16 - (plaintext.size() % 16);
    std::vector<uint8_t> padded_plaintext = plaintext;
    for (size_t i = 0; i < padding; ++i) {
        padded_plaintext.push_back(padding);
    }

    ciphertext.resize(padded_plaintext.size());

    for (size_t i = 0; i < padded_plaintext.size(); i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, padded_plaintext.data() + i, ciphertext.data() + i);
    }

    mbedtls_aes_free(&aes);

    return ciphertext;
}

} // namespace e7_switcher

#else // ESP_PLATFORM

#include <openssl/evp.h>
#include <openssl/aes.h>

namespace e7_switcher {

std::vector<uint8_t> aes_decrypt(const std::vector<uint8_t>& ciphertext, const std::string& key) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    std::vector<uint8_t> plaintext(ciphertext.size());

    if(!(ctx = EVP_CIPHER_CTX_new())) throw std::runtime_error("Failed to create new cipher context");

    auto key_len_fn = EVP_aes_128_ecb;
    if (key.length() == 24) {
        key_len_fn = EVP_aes_192_ecb;
    }
    else if (key.length() == 32) {
        key_len_fn = EVP_aes_256_ecb;
    }
    
    if(1 != EVP_DecryptInit_ex(ctx, key_len_fn(), NULL, (const unsigned char*)key.c_str(), NULL)) throw std::runtime_error("Failed to initialize decryption");
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    if(1 != EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size())) throw std::runtime_error("Failed to decrypt update");
    plaintext_len = len;

    if(1 != EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) throw std::runtime_error("Failed to finalize decryption");
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    plaintext.resize(plaintext_len);
    return plaintext;
}

std::vector<uint8_t> aes_encrypt(const std::vector<uint8_t>& plaintext, const std::string& key) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;
    std::vector<uint8_t> ciphertext(plaintext.size() + AES_BLOCK_SIZE);

    if(!(ctx = EVP_CIPHER_CTX_new())) throw std::runtime_error("Failed to create new cipher context");

    auto key_len_fn = EVP_aes_128_ecb;
    if (key.length() == 24) {
        key_len_fn = EVP_aes_192_ecb;
    }
    else if (key.length() == 32) {
        key_len_fn = EVP_aes_256_ecb;
    }
    
    if(1 != EVP_EncryptInit_ex(ctx, key_len_fn(), NULL, (const unsigned char*)key.c_str(), NULL)) throw std::runtime_error("Failed to initialize encryption");
    
    if(1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size())) throw std::runtime_error("Failed to encrypt update");
    ciphertext_len = len;

    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) throw std::runtime_error("Failed to finalize encryption");
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    ciphertext.resize(ciphertext_len);
    return ciphertext;
}

} // namespace e7_switcher

#endif // ESP_PLATFORM
