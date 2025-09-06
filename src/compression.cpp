#include "e7-switcher/compression.h"
#include <zlib.h>
#include "e7-switcher/logger.h"
#include <stdexcept>

namespace e7_switcher {

std::vector<uint8_t> compress_data(const std::vector<uint8_t>& data, int level) {
    if (data.empty()) {
        return {};
    }

    auto& logger = Logger::instance();
    
    // Ensure compression level is within valid range
    if (level < 0) level = 0;
    if (level > 9) level = 9;  // zlib uses 0-9 compression levels
    
    // Estimate the maximum compressed size (worst case)
    uLong max_compressed_size = compressBound(data.size());
    std::vector<uint8_t> compressed_data(max_compressed_size);
    
    // Perform compression
    uLongf compressed_size = max_compressed_size;
    int status = compress2(
        compressed_data.data(), 
        &compressed_size, 
        data.data(), 
        data.size(), 
        level
    );
    
    if (status != Z_OK) {
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
    auto src = compressed_data.data();
    auto src_len = compressed_data.size();
    if (!src || src_len == 0) {
        throw std::runtime_error("Invalid input data");
    }
    z_stream strm{};
    strm.next_in  = const_cast<Bytef*>(src);
    strm.avail_in = static_cast<uInt>(src_len);

    // 16 + 15 => accept gzip stream (GZIP header + max window)
    // If you want auto zlib/gzip detection, use (32 + MAX_WBITS) instead.
    int rc = inflateInit2(&strm, 16 + MAX_WBITS);
    if (rc != Z_OK) {
        throw std::runtime_error("Decompression failed");
    }

    std::string out;
    std::vector<unsigned char> buf(64 * 1024);

    int status = Z_OK;
    while (status != Z_STREAM_END) {
        strm.next_out  = buf.data();
        strm.avail_out = static_cast<uInt>(buf.size());

        status = inflate(&strm, Z_NO_FLUSH);
        if (status != Z_OK && status != Z_STREAM_END) {
            inflateEnd(&strm);
            throw std::runtime_error("Decompression failed");
        }

        size_t produced = buf.size() - strm.avail_out;
        out.append(reinterpret_cast<const char*>(buf.data()), produced);
    }

    inflateEnd(&strm);
    return std::vector<uint8_t>(out.begin(), out.end());
}
} // namespace e7_switcher
