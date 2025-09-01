#include "stream_message.h"
#include "constants.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>

namespace e7_switcher {

namespace {
inline uint16_t le16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
}

StreamMessage::StreamMessage() : sock_(-1) {}

StreamMessage::~StreamMessage() {
    // We don't close the socket here as it's managed by the caller
}

void StreamMessage::connect(int sock) {
    sock_ = sock;
    inbuf_.clear();
}

void StreamMessage::disconnect() {
    sock_ = -1;
    inbuf_.clear();
}

void StreamMessage::send_message(const std::vector<uint8_t>& data) {
    if (sock_ == -1) throw std::runtime_error("Not connected");
    const uint8_t* p = data.data();
    size_t left = data.size();
    while (left > 0) {
        ssize_t n = ::send(sock_, p, left, 0);
        if (n < 0) throw std::runtime_error("Send failed");
        p += n;
        left -= static_cast<size_t>(n);
    }
}

std::vector<uint8_t> StreamMessage::receive_message(int timeout_ms) {
    if (sock_ == -1) throw std::runtime_error("Not connected");

    // Temporarily extend SO_RCVTIMEO for this call; restore after.
    struct timeval oldtv{}, newtv{};
    socklen_t optlen = sizeof(oldtv);
    getsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &oldtv, &optlen);
    newtv.tv_sec  = timeout_ms / 1000;
    newtv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&newtv, sizeof newtv);

    std::vector<uint8_t> out;

    // Try extracting if already buffered
    if (try_extract_one_packet(out)) {
        // Restore old timeout and return
        setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&oldtv, sizeof oldtv);
        return out;
    }

    // Keep reading until a full packet is available or timeout hits
    const size_t READ_CHUNK = 4096;
    std::vector<uint8_t> tmp;
    tmp.resize(READ_CHUNK);

    while (true) {
        ssize_t n = ::recv(sock_, tmp.data(), READ_CHUNK, 0);
        if (n < 0) {
            // respect socket's SO_RCVTIMEO (errno == EAGAIN/EWOULDBLOCK typically)
            setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&oldtv, sizeof oldtv);
            throw std::runtime_error("Receive timeout or error");
        } else if (n == 0) {
            setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&oldtv, sizeof oldtv);
            throw std::runtime_error("Peer closed connection");
        }
        inbuf_.insert(inbuf_.end(), tmp.begin(), tmp.begin() + n);

        if (try_extract_one_packet(out)) {
            setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&oldtv, sizeof oldtv);
            return out;
        }
        // otherwise, loop to read more bytes
    }
}

bool StreamMessage::try_extract_one_packet(std::vector<uint8_t>& out) {
    // Search for start-of-header (FE F0)
    size_t i = 0;
    while (true) {
        // need at least header size to proceed
        if (inbuf_.size() - i < HEADER_SIZE) {
            // drop garbage before i (if any), but keep partial header in buffer
            if (i > 0) inbuf_.erase(inbuf_.begin(), inbuf_.begin() + i);
            return false;
        }

        // Look for start marker
        uint16_t maybe_marker = le16(&inbuf_[i]);
        if (maybe_marker == MAGIC1) {
            // Verify header tail markers present (we have >= HEADER_SIZE here)
            uint16_t maybe_tail = le16(&inbuf_[i + 38]);
            if (maybe_tail == MAGIC2) {
                // Parse total length (little-endian) from bytes [2..3]
                uint16_t total_len = le16(&inbuf_[i + 2]);

                // Sanity check: header+crc minimum
                if (total_len < HEADER_SIZE + CRC_TAIL_SIZE) {
                    // corrupt; skip one byte and continue searching
                    ++i;
                    continue;
                }

                // If the full packet isn't yet buffered, wait for more bytes
                if (inbuf_.size() - i < total_len) {
                    // keep what's before i? it's not a valid header, but i points to a plausible header start
                    if (i > 0) inbuf_.erase(inbuf_.begin(), inbuf_.begin() + i);
                    return false;
                }

                // Extract packet
                out.assign(inbuf_.begin() + i, inbuf_.begin() + i + total_len);
                // Erase consumed bytes (including any junk before header)
                inbuf_.erase(inbuf_.begin(), inbuf_.begin() + i + total_len);
                return true;
            } else {
                // Not a valid header end; move forward one byte
                ++i;
            }
        } else {
            ++i;
        }
    }
}

} // namespace e7_switcher
