#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include "parser.h"

namespace e7_switcher {

class MessageStream {
public:
    MessageStream();
    ~MessageStream();

    // Connect/disconnect
    void connect(int sock);
    void disconnect();

    // Send/receive methods
    void send_message(const std::vector<uint8_t>& data);
    ProtocolMessage receive_message(int timeout_ms = 15000); // long, per-call timeout

private:
    // Stream helpers
    bool recv_into_buffer_until(size_t min_size, int timeout_ms);
    bool try_extract_one_packet(std::vector<uint8_t>& out);

    int sock_;
    
    // Incoming stream buffer
    std::vector<uint8_t> inbuf_;
};

} // namespace e7_switcher
