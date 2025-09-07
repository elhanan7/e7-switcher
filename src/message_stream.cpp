#include "e7-switcher/message_stream.h"
#include "e7-switcher/constants.h"
#include "e7-switcher/parser.h"
#include "e7-switcher/socket_utils.h"
#include <stdexcept>
#include <iostream>
#include <cstring>

namespace e7_switcher {

namespace {
inline uint16_t le16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
}

MessageStream::MessageStream() : host_(""), port_(0), timeout_(5), recv_timeout_seconds_(5), sock_(net::INVALID_SOCKET_HANDLE) {}

MessageStream::~MessageStream() {
    close();
}

void MessageStream::connect_to_server(const std::string& host, int port, int timeout_seconds) {
    host_ = host;
    port_ = port;
    timeout_ = timeout_seconds;
    std::string err;
    if (!net::startup(err)) {
        throw std::runtime_error("Socket startup failed: " + err);
    }
    // Close existing if any and connect
    if (sock_ != net::INVALID_SOCKET_HANDLE) {
        net::close(sock_);
    }
    if (!net::connect(sock_, host_, port_, timeout_seconds * 1000, err)) {
        throw std::runtime_error("Connection Failed: " + err);
    }
    // Set default recv timeout
    if (!net::set_recv_timeout(sock_, timeout_seconds, err)) {
        // Not fatal for connect, but keep state
        // We still store it so future overrides know our intent
    }
    recv_timeout_seconds_ = timeout_seconds;
    
    // Clear the input buffer
    inbuf_.clear();
}

void MessageStream::close() {
    if (sock_ != net::INVALID_SOCKET_HANDLE) {
        net::close(sock_);
        inbuf_.clear();
    }
}

bool MessageStream::is_connected() const {
    return sock_ != net::INVALID_SOCKET_HANDLE;
}

void MessageStream::create_socket() {
    // Close existing socket if any, then create a new one via utils
    if (sock_ != net::INVALID_SOCKET_HANDLE) {
        net::close(sock_);
    }
    std::string err;
    if (!net::create_tcp_socket(sock_, err)) {
        throw std::runtime_error("Failed to create socket: " + err);
    }
}

void MessageStream::set_socket_timeout(int timeout_seconds) {
    std::string err;
    (void)net::set_recv_timeout(sock_, timeout_seconds, err);
    recv_timeout_seconds_ = timeout_seconds;
}


void MessageStream::send_message(const std::vector<uint8_t>& data) {
    if (sock_ == net::INVALID_SOCKET_HANDLE) throw std::runtime_error("Not connected");
    std::string err;
    if (!net::send_all(sock_, data, err)) {
        throw std::runtime_error("Send failed: " + err);
    }
}

void MessageStream::send_message(const ProtocolMessage& message) {
    // Assemble the complete message bytes from the ProtocolMessage object
    std::vector<uint8_t> data;
    
    // Add header
    data.insert(data.end(), message.raw_header.begin(), message.raw_header.end());
    
    // Add payload
    data.insert(data.end(), message.payload.begin(), message.payload.end());
    
    // Add CRC
    data.insert(data.end(), message.crc.begin(), message.crc.end());
    
    // Send the assembled message
    send_message(data);
}

ProtocolMessage MessageStream::receive_message(int timeout_ms) {
    if (sock_ == net::INVALID_SOCKET_HANDLE) throw std::runtime_error("Not connected");

    // Temporarily override receive timeout using seconds resolution
    int old_timeout = recv_timeout_seconds_;
    int tmp_timeout_sec = (timeout_ms + 999) / 1000; // ceil to seconds
    std::string err;
    (void)net::set_recv_timeout(sock_, tmp_timeout_sec, err);
    recv_timeout_seconds_ = tmp_timeout_sec;

    std::vector<uint8_t> out;

    // Try extracting if already buffered
    if (try_extract_one_packet(out)) {
        // Restore old timeout and return
        (void)net::set_recv_timeout(sock_, old_timeout, err);
        recv_timeout_seconds_ = old_timeout;
        return parse_protocol_packet(out);
    }

    // Keep reading until a full packet is available or timeout hits
    const size_t READ_CHUNK = 4096;
    std::vector<uint8_t> tmp;
    tmp.resize(READ_CHUNK);

    while (true) {
        int n = net::recv_some(sock_, tmp.data(), READ_CHUNK, err);
        if (n < 0) {
            // -2 => timeout; -1 => error
            (void)net::set_recv_timeout(sock_, old_timeout, err);
            recv_timeout_seconds_ = old_timeout;
            throw std::runtime_error("Receive timeout or error");
        } else if (n == 0) {
            (void)net::set_recv_timeout(sock_, old_timeout, err);
            recv_timeout_seconds_ = old_timeout;
            throw std::runtime_error("Peer closed connection");
        }
        inbuf_.insert(inbuf_.end(), tmp.begin(), tmp.begin() + n);

        if (try_extract_one_packet(out)) {
            (void)net::set_recv_timeout(sock_, old_timeout, err);
            recv_timeout_seconds_ = old_timeout;
            return parse_protocol_packet(out);
        }
        // otherwise, loop to read more bytes
    }
}

bool MessageStream::try_extract_one_packet(std::vector<uint8_t>& out) {
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
