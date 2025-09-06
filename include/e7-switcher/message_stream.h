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

    // Connection management
    void connect_to_server(const std::string& host, int port, int timeout_seconds);
    void close();
    bool is_connected() const;

    // Send/receive methods
    void send_message(const std::vector<uint8_t>& data);
    void send_message(const ProtocolMessage& message);
    ProtocolMessage receive_message(int timeout_ms = 15000); // long, per-call timeout

private:
    // Stream helpers
    bool recv_into_buffer_until(size_t min_size, int timeout_ms);
    bool try_extract_one_packet(std::vector<uint8_t>& out);

    // Socket management
    void create_socket();
    void set_socket_timeout(int timeout_seconds);
    
    std::string host_;
    int port_;
    int timeout_;
    int sock_;
    
    // Incoming stream buffer
    std::vector<uint8_t> inbuf_;
};

} // namespace e7_switcher
