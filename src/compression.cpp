#include "compression.h"
#include "miniz.h"
#include "logger.h"
#include <stdexcept>

namespace e7_switcher {

std::vector<uint8_t> compress_data(const std::vector<uint8_t>& data, int level) {
    if (data.empty()) {
        return {};
    }

    auto& logger = Logger::instance();
    
    // Ensure compression level is within valid range
    if (level < 0) level = 0;
    if (level > 10) level = 10;
    
    // Estimate the maximum compressed size (worst case)
    size_t max_compressed_size = mz_compressBound(data.size());
    std::vector<uint8_t> compressed_data(max_compressed_size);
    
    // Perform compression
    mz_ulong compressed_size = max_compressed_size;
    int status = mz_compress2(
        compressed_data.data(), 
        &compressed_size, 
        data.data(), 
        data.size(), 
        level
    );
    
    if (status != MZ_OK) {
        logger.errorf("Compression failed with status %d", status);
        throw std::runtime_error("Compression failed");
    }
    
    // Resize the output vector to the actual compressed size
    compressed_data.resize(compressed_size);
    logger.debugf("Compressed %zu bytes to %zu bytes (ratio: %.2f%%)", 
                 data.size(), compressed_size, 
                 (float)compressed_size / data.size() * 100.0f);
    
    return compressed_data;
}

std::vector<uint8_t> decompress_data(const std::vector<uint8_t>& compressed_data) {
    if (compressed_data.empty()) {
        return {};
    }

    auto& logger = Logger::instance();
    
    // Start with an estimated decompressed size
    // This is a guess - we'll resize if needed
    size_t decompressed_size = compressed_data.size() * 4;
    std::vector<uint8_t> decompressed_data(decompressed_size);
    
    // Try to decompress
    mz_ulong actual_size = decompressed_size;
    int status = mz_uncompress(
        decompressed_data.data(), 
        &actual_size, 
        compressed_data.data(), 
        compressed_data.size()
    );
    
    // If we didn't allocate enough space, try again with more space
    if (status == MZ_BUF_ERROR) {
        // Try with a much larger buffer
        decompressed_size *= 4;
        decompressed_data.resize(decompressed_size);
        actual_size = decompressed_size;
        
        status = mz_uncompress(
            decompressed_data.data(), 
            &actual_size, 
            compressed_data.data(), 
            compressed_data.size()
        );
    }
    
    if (status != MZ_OK) {
        logger.errorf("Decompression failed with status %d", status);
        throw std::runtime_error("Decompression failed");
    }
    
    // Resize the output vector to the actual decompressed size
    decompressed_data.resize(actual_size);
    logger.debugf("Decompressed %zu bytes to %zu bytes", 
                 compressed_data.size(), actual_size);
    
    return decompressed_data;
}

} // namespace e7_switcher
