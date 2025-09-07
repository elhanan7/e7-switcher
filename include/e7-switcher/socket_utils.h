#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace e7_switcher {
namespace net {

#ifdef _WIN32
  using SocketHandle = unsigned long long; // large enough to hold SOCKET
  constexpr SocketHandle INVALID_SOCKET_HANDLE = static_cast<SocketHandle>(~0ull);
#else
  using SocketHandle = int;
  constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;
#endif

// Initialize socket subsystem (no-op on POSIX; WSAStartup on Windows). Returns true on success.
bool startup(std::string& err);
// Cleanup socket subsystem (no-op on POSIX; WSACleanup on Windows).
void cleanup();

// Create a TCP socket handle (AF_INET, SOCK_STREAM). Returns true on success.
bool create_tcp_socket(SocketHandle& out, std::string& err);

// Connect to host:port with timeout in milliseconds. Host may be IPv4 string or hostname.
// On success, returns true and assigns a valid SocketHandle to out. On failure, returns false and sets err.
bool connect(SocketHandle& out, const std::string& host, int port, int timeout_ms, std::string& err);

// Close socket safely (idempotent). After close, handle becomes INVALID_SOCKET_HANDLE.
void close(SocketHandle& s);

// Set SO_RCVTIMEO. On POSIX this is seconds+usec; on Windows it's milliseconds. Here we accept seconds resolution.
bool set_recv_timeout(SocketHandle s, int timeout_seconds, std::string& err);

// Send all bytes in buffer. Returns true on success; false on error (err populated).
bool send_all(SocketHandle s, const uint8_t* data, size_t len, std::string& err);
inline bool send_all(SocketHandle s, const std::vector<uint8_t>& v, std::string& err) {
  return send_all(s, v.data(), v.size(), err);
}

// Receive up to max_len bytes. Return values:
//   >= 0 : number of bytes received (0 means peer closed)
//   -1   : error (err populated)
//   -2   : timeout (err may be empty)
int recv_some(SocketHandle s, uint8_t* buf, size_t max_len, std::string& err);

// Receive exactly n bytes (unless error/timeout). Returns true on success; false otherwise.
bool recv_exact(SocketHandle s, size_t n, std::vector<uint8_t>& out, std::string& err);

} // namespace net
} // namespace e7_switcher
