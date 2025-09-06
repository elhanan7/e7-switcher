#ifndef E7_COMPRESSION_H
#define E7_COMPRESSION_H

#include <vector>
#include <cstdint>

namespace e7_switcher {

/**
 * Compress data using the zlib library
 * 
 * @param data The data to compress
 * @param level Compression level (0-9, 0=no compression, 9=max compression)
 * @return Compressed data as a vector of bytes
 */
std::vector<uint8_t> compress_data(const std::vector<uint8_t>& data, int level = 6);

/**
 * Decompress data using the zlib library
 * 
 * @param compressed_data The compressed data
 * @return Decompressed data as a vector of bytes
 */
std::vector<uint8_t> decompress_data(const std::vector<uint8_t>& compressed_data);

} // namespace e7_switcher

#endif // E7_COMPRESSION_H
