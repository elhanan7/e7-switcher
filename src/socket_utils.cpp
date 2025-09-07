#include "e7-switcher/socket_utils.h"

#include <cstring>
#include <string>
#include <cstdio>

#ifdef _WIN32
  #define NOMINMAX
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "Ws2_32.lib")
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <netdb.h>
  #include <errno.h>
#endif

namespace e7_switcher {
namespace net {

#ifdef _WIN32
  using SysSocket = SOCKET;
  static SysSocket to_sys(SocketHandle h) { return (SysSocket)(uintptr_t)h; }
  static SocketHandle from_sys(SysSocket s) { return (SocketHandle)(uintptr_t)s; }
  static bool is_invalid_sys(SysSocket s) { return s == INVALID_SOCKET; }
#else
  using SysSocket = int;
  static SysSocket to_sys(SocketHandle h) { return (SysSocket)h; }
  static SocketHandle from_sys(SysSocket s) { return (SocketHandle)s; }
  static bool is_invalid_sys(SysSocket s) { return s < 0; }
#endif

static std::string last_error_string() {
#ifdef _WIN32
    int code = WSAGetLastError();
    char* buf = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buf, 0, NULL);
    std::string msg = buf ? buf : "winsock error";
    if (buf) LocalFree(buf);
    return msg;
#else
    return std::string(strerror(errno));
#endif
}

bool startup(std::string& err) {
#ifdef _WIN32
    static bool initialized = false;
    if (initialized) return true;
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) { err = "WSAStartup failed: " + std::to_string(res); return false; }
    initialized = true;
#endif
    (void)err;
    return true;
}

void cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

bool create_tcp_socket(SocketHandle& out, std::string& err) {
#ifdef _WIN32
    SysSocket s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (is_invalid_sys(s)) { err = last_error_string(); out = INVALID_SOCKET_HANDLE; return false; }
    out = from_sys(s);
    return true;
#else
    SysSocket s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (is_invalid_sys(s)) { err = last_error_string(); out = INVALID_SOCKET_HANDLE; return false; }
    out = from_sys(s);
    return true;
#endif
}

static bool set_nonblocking(SysSocket s, bool nb, std::string& err) {
#ifdef _WIN32
    u_long mode = nb ? 1u : 0u;
    if (ioctlsocket(s, FIONBIO, &mode) != 0) { err = last_error_string(); return false; }
    return true;
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) { err = last_error_string(); return false; }
    if (nb)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;
    if (fcntl(s, F_SETFL, flags) < 0) { err = last_error_string(); return false; }
    return true;
#endif
}

bool connect(SocketHandle& out, const std::string& host, int port, int timeout_ms, std::string& err) {
    out = INVALID_SOCKET_HANDLE;
    if (timeout_ms < 0) timeout_ms = 0;

    // Resolve host (IPv4 only for now)
    struct addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo* result = nullptr;
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
#ifdef _WIN32
    int gairet = getaddrinfo(host.c_str(), port_str, &hints, &result);
    if (gairet != 0) { err = "getaddrinfo failed"; return false; }
#else
    int gairet = getaddrinfo(host.c_str(), port_str, &hints, &result);
    if (gairet != 0) { err = std::string("getaddrinfo failed: ") + std::to_string(gairet); return false; }
#endif

    std::string last_err;
    for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        SysSocket s = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (is_invalid_sys(s)) { last_err = last_error_string(); continue; }

        std::string nb_err;
        if (!set_nonblocking(s, true, nb_err)) { last_err = nb_err; 
#ifdef _WIN32
            closesocket(s);
#else
            ::close(s);
#endif
            continue; }

        int ret = ::connect(s, rp->ai_addr, (int)rp->ai_addrlen);
        if (ret == 0) {
            // immediate connect
        } else {
#ifdef _WIN32
            int werr = WSAGetLastError();
            if (werr != WSAEWOULDBLOCK && werr != WSAEINPROGRESS && werr != WSAEINVAL) {
                last_err = last_error_string();
#ifdef _WIN32
                closesocket(s);
#else
                ::close(s);
#endif
                continue;
            }
#else
            if (errno != EINPROGRESS) {
                last_err = last_error_string();
#ifdef _WIN32
                closesocket(s);
#else
                ::close(s);
#endif
                continue;
            }
#endif
            // wait for writability
            fd_set wfds; FD_ZERO(&wfds); FD_SET(s, &wfds);
            struct timeval tv; tv.tv_sec = timeout_ms / 1000; tv.tv_usec = (timeout_ms % 1000) * 1000;
            int sel = select((int)(s+1), nullptr, &wfds, nullptr, &tv);
            if (sel <= 0) {
                last_err = (sel == 0) ? std::string("connect timeout") : last_error_string();
#ifdef _WIN32
                closesocket(s);
#else
                ::close(s);
#endif
                continue;
            }
            // Check SO_ERROR
            int soerr = 0;
#ifdef _WIN32
            int slen = (int)sizeof(soerr);
#else
            socklen_t slen = sizeof(soerr);
#endif
            getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&soerr, &slen);
            if (soerr != 0) {
#ifdef _WIN32
                last_err = std::string("SO_ERROR=") + std::to_string(soerr);
#else
                last_err = std::string(strerror(soerr));
#endif
#ifdef _WIN32
                closesocket(s);
#else
                ::close(s);
#endif
                continue;
            }
        }

        // restore blocking
        std::string rb_err;
        if (!set_nonblocking(s, false, rb_err)) {
            last_err = rb_err;
#ifdef _WIN32
            closesocket(s);
#else
            ::close(s);
#endif
            continue;
        }

        out = from_sys(s);
        freeaddrinfo(result);
        return true;
    }

    if (result) freeaddrinfo(result);
    err = last_err.empty() ? std::string("connect failed") : last_err;
    return false;
}

void close(SocketHandle& s) {
#ifdef _WIN32
    SysSocket ss = to_sys(s);
    if (!is_invalid_sys(ss)) { closesocket(ss); }
#else
    SysSocket ss = to_sys(s);
    if (!is_invalid_sys(ss)) { ::close(ss); }
#endif
    s = INVALID_SOCKET_HANDLE;
}

bool set_recv_timeout(SocketHandle h, int timeout_seconds, std::string& err) {
#ifdef _WIN32
    DWORD tv = (timeout_seconds < 0) ? 0 : (DWORD)(timeout_seconds * 1000);
    if (setsockopt(to_sys(h), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) != 0) { err = last_error_string(); return false; }
    return true;
#else
    struct timeval tv{};
    tv.tv_sec = timeout_seconds;
    tv.tv_usec = 0;
    if (setsockopt(to_sys(h), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) != 0) { err = last_error_string(); return false; }
    return true;
#endif
}

bool send_all(SocketHandle h, const uint8_t* data, size_t len, std::string& err) {
    size_t sent = 0;
    SysSocket s = to_sys(h);
    while (sent < len) {
#ifdef _WIN32
        int n = ::send(s, (const char*)data + sent, (int)(len - sent), 0);
#else
        ssize_t n = ::send(s, (const char*)data + sent, (size_t)(len - sent), 0);
#endif
        if (n < 0) {
#ifdef _WIN32
            int e = WSAGetLastError();
            if (e == WSAEINTR) continue;
#else
            if (errno == EINTR) continue;
#endif
            err = last_error_string();
            return false;
        }
        if (n == 0) { err = "send returned 0"; return false; }
        sent += (size_t)n;
    }
    return true;
}

int recv_some(SocketHandle h, uint8_t* buf, size_t max_len, std::string& err) {
    SysSocket s = to_sys(h);
#ifdef _WIN32
    int n = ::recv(s, (char*)buf, (int)max_len, 0);
    if (n > 0) return n;
    if (n == 0) return 0; // peer closed
    int e = WSAGetLastError();
    if (e == WSAEWOULDBLOCK || e == WSAETIMEDOUT) return -2; // timeout
    if (e == WSAEINTR) return 0; // retryable; treat as 0 bytes
    err = last_error_string();
    return -1;
#else
    ssize_t n = ::recv(s, (char*)buf, max_len, 0);
    if (n > 0) return (int)n;
    if (n == 0) return 0; // peer closed
    if (errno == EAGAIN || errno == EWOULDBLOCK) return -2; // timeout
    if (errno == EINTR) return 0; // no data this time
    err = last_error_string();
    return -1;
#endif
}

bool recv_exact(SocketHandle h, size_t nbytes, std::vector<uint8_t>& out, std::string& err) {
    out.clear();
    out.reserve(nbytes);
    std::vector<uint8_t> tmp(4096);
    size_t remaining = nbytes;
    while (remaining > 0) {
        size_t want = remaining < tmp.size() ? remaining : tmp.size();
        int n = recv_some(h, tmp.data(), want, err);
        if (n > 0) {
            out.insert(out.end(), tmp.begin(), tmp.begin() + n);
            remaining -= (size_t)n;
            continue;
        } else if (n == 0) {
            err = "peer closed";
            return false;
        } else if (n == -2) {
            // timeout
            err = err.empty() ? std::string("receive timeout") : err;
            return false;
        } else {
            // error
            return false;
        }
    }
    return true;
}

} // namespace net
} // namespace e7_switcher
