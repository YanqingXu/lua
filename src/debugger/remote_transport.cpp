/**
 * @file remote_transport.cpp
 * @brief Windows/POSIX TCP wrapper for the opt-in remote debugger.
 */

#include "debugger/remote_transport.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

namespace Lua::Debugger::Remote {

namespace {

#ifdef _WIN32
using NativeSocket = SOCKET;
constexpr NativeSocket kInvalidSocket = INVALID_SOCKET;

class WinsockLifetime {
public:
    WinsockLifetime() {
        WSADATA data{};
        ready = WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }
    ~WinsockLifetime() {
        if (ready) {
            WSACleanup();
        }
    }
    bool ready = false;
};

bool socketsReady() {
    static WinsockLifetime lifetime;
    return lifetime.ready;
}

void closeSocket(NativeSocket socket) noexcept {
    if (socket != kInvalidSocket) {
        closesocket(socket);
    }
}

i32 lastSocketError() noexcept {
    return WSAGetLastError();
}
#else
using NativeSocket = int;
constexpr NativeSocket kInvalidSocket = -1;

bool socketsReady() {
    return true;
}

void closeSocket(NativeSocket socket) noexcept {
    if (socket != kInvalidSocket) {
        ::close(socket);
    }
}

i32 lastSocketError() noexcept {
    return errno;
}
#endif

NativeSocket native(isize value) noexcept {
    return static_cast<NativeSocket>(value);
}

isize stored(NativeSocket value) noexcept {
    return static_cast<isize>(value);
}

ProtocolError socketFailure(StrView operation) {
    return {ProtocolStatus::ProtocolError,
            Str(operation) + " failed (socket error " + std::to_string(lastSocketError()) + ")", 0};
}

ProtocolError receiveFailure() {
    const i32 error = lastSocketError();
#ifdef _WIN32
    const bool timedOut = error == WSAETIMEDOUT || error == WSAEWOULDBLOCK;
#else
    const bool timedOut = error == EAGAIN || error == EWOULDBLOCK;
#endif
    return {timedOut ? ProtocolStatus::Timeout : ProtocolStatus::ProtocolError,
            Str("debug receive failed (socket error ") + std::to_string(error) + ")", 0};
}

ProtocolResult<sockaddr_in> ipv4Endpoint(StrView address, u16 port) {
    sockaddr_in endpoint{};
    endpoint.sin_family = AF_INET;
    endpoint.sin_port = htons(port);
    const Str terminated(address);
    if (inet_pton(AF_INET, terminated.c_str(), &endpoint.sin_addr) != 1) {
        return std::unexpected(
            ProtocolError{ProtocolStatus::InvalidArgument, "debug transport address must be an IPv4 literal", 0});
    }
    return endpoint;
}

} // namespace

bool isLoopbackAddress(StrView address) noexcept {
    return address == "127.0.0.1" || address.starts_with("127.");
}

TcpConnection::~TcpConnection() {
    close();
}

TcpConnection::TcpConnection(TcpConnection&& other) noexcept
    : socket_(other.socket_.exchange(isize{-1}, std::memory_order_acq_rel)) {}

TcpConnection& TcpConnection::operator=(TcpConnection&& other) noexcept {
    if (this != &other) {
        close();
        socket_.store(other.socket_.exchange(isize{-1}, std::memory_order_acq_rel), std::memory_order_release);
    }
    return *this;
}

ProtocolResult<TcpConnection> TcpConnection::connect(StrView address, u16 port, u32 timeoutMs) {
    if (!socketsReady()) {
        return std::unexpected(socketFailure("socket initialization"));
    }
    auto endpoint = ipv4Endpoint(address, port);
    if (!endpoint || port == 0) {
        return std::unexpected(endpoint ? ProtocolError{ProtocolStatus::InvalidArgument, "debug port must be non-zero", 0}
                                        : endpoint.error());
    }
    const NativeSocket socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == kInvalidSocket) {
        return std::unexpected(socketFailure("socket creation"));
    }
    TcpConnection result(stored(socket));
    if (auto configured = result.setTimeouts(timeoutMs, timeoutMs); !configured) {
        return std::unexpected(configured.error());
    }
    if (::connect(socket, reinterpret_cast<const sockaddr*>(&*endpoint), sizeof(*endpoint)) != 0) {
        return std::unexpected(socketFailure("debug connection"));
    }
    return result;
}

bool TcpConnection::valid() const noexcept {
    return socket_.load(std::memory_order_acquire) != -1;
}

ProtocolResult<void> TcpConnection::sendAll(std::span<const u8> bytes) {
    if (!valid()) {
        return std::unexpected(ProtocolError{ProtocolStatus::InvalidState, "debug connection is closed", 0});
    }
    usize offset = 0;
    while (offset < bytes.size()) {
        const isize socket = socket_.load(std::memory_order_acquire);
        if (socket == -1) {
            return std::unexpected(ProtocolError{ProtocolStatus::InvalidState, "debug connection is closed", 0});
        }
        const usize remaining = bytes.size() - offset;
        const i32 chunk = static_cast<i32>(std::min(remaining, static_cast<usize>(std::numeric_limits<i32>::max())));
#ifdef _WIN32
        const i32 sent = ::send(native(socket), reinterpret_cast<const char*>(bytes.data() + offset), chunk, 0);
#else
        const i32 sent = static_cast<i32>(::send(native(socket), bytes.data() + offset, static_cast<usize>(chunk), 0));
#endif
        if (sent <= 0) {
            return std::unexpected(socketFailure("debug send"));
        }
        offset += static_cast<usize>(sent);
    }
    return {};
}

ProtocolResult<Vec<u8>> TcpConnection::receiveSome(usize maxBytes) {
    if (!valid()) {
        return std::unexpected(ProtocolError{ProtocolStatus::InvalidState, "debug connection is closed", 0});
    }
    const usize bounded = std::clamp(maxBytes, usize{1}, kProtocolMaxFrameBytes);
    Vec<u8> bytes(bounded);
    const isize socket = socket_.load(std::memory_order_acquire);
    if (socket == -1) {
        return std::unexpected(ProtocolError{ProtocolStatus::InvalidState, "debug connection is closed", 0});
    }
#ifdef _WIN32
    const i32 received = ::recv(native(socket), reinterpret_cast<char*>(bytes.data()), static_cast<i32>(bounded), 0);
#else
    const i32 received = static_cast<i32>(::recv(native(socket), bytes.data(), bounded, 0));
#endif
    if (received == 0) {
        return std::unexpected(ProtocolError{ProtocolStatus::Terminated, "debug connection closed by peer", 0});
    }
    if (received < 0) {
        return std::unexpected(receiveFailure());
    }
    bytes.resize(static_cast<usize>(received));
    return bytes;
}

ProtocolResult<void> TcpConnection::setTimeouts(u32 receiveTimeoutMs, u32 sendTimeoutMs) {
    const isize socket = socket_.load(std::memory_order_acquire);
    if (socket == -1) {
        return std::unexpected(ProtocolError{ProtocolStatus::InvalidState, "debug connection is closed", 0});
    }
#ifdef _WIN32
    const DWORD receiveTimeout = receiveTimeoutMs;
    const DWORD sendTimeout = sendTimeoutMs;
    if (setsockopt(native(socket), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&receiveTimeout),
                   sizeof(receiveTimeout)) != 0 ||
        setsockopt(native(socket), SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&sendTimeout),
                   sizeof(sendTimeout)) != 0) {
#else
    const timeval receiveTimeout{static_cast<time_t>(receiveTimeoutMs / 1000U),
                                 static_cast<suseconds_t>((receiveTimeoutMs % 1000U) * 1000U)};
    const timeval sendTimeout{static_cast<time_t>(sendTimeoutMs / 1000U),
                              static_cast<suseconds_t>((sendTimeoutMs % 1000U) * 1000U)};
    if (setsockopt(native(socket), SOL_SOCKET, SO_RCVTIMEO, &receiveTimeout, sizeof(receiveTimeout)) != 0 ||
        setsockopt(native(socket), SOL_SOCKET, SO_SNDTIMEO, &sendTimeout, sizeof(sendTimeout)) != 0) {
#endif
        return std::unexpected(socketFailure("debug socket timeout configuration"));
    }
    return {};
}

void TcpConnection::close() noexcept {
    const isize socket = socket_.exchange(isize{-1}, std::memory_order_acq_rel);
    if (socket != -1) {
#ifdef _WIN32
        shutdown(native(socket), SD_BOTH);
#else
        shutdown(native(socket), SHUT_RDWR);
#endif
        closeSocket(native(socket));
    }
}

TcpListener::~TcpListener() {
    close();
}

TcpListener::TcpListener(TcpListener&& other) noexcept
    : socket_(other.socket_.exchange(isize{-1}, std::memory_order_acq_rel)),
      localPort_(std::exchange(other.localPort_, u16{0})) {}

TcpListener& TcpListener::operator=(TcpListener&& other) noexcept {
    if (this != &other) {
        close();
        socket_.store(other.socket_.exchange(isize{-1}, std::memory_order_acq_rel), std::memory_order_release);
        localPort_ = std::exchange(other.localPort_, u16{0});
    }
    return *this;
}

ProtocolResult<TcpListener> TcpListener::listen(StrView address, u16 port, i32 backlog) {
    if (!socketsReady()) {
        return std::unexpected(socketFailure("socket initialization"));
    }
    auto endpoint = ipv4Endpoint(address, port);
    if (!endpoint) {
        return std::unexpected(endpoint.error());
    }
    const NativeSocket socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == kInvalidSocket) {
        return std::unexpected(socketFailure("listener creation"));
    }
    TcpListener result;
    result.socket_.store(stored(socket), std::memory_order_release);
    const i32 enabled = 1;
#ifdef _WIN32
    (void)setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&enabled), sizeof(enabled));
#else
    (void)setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
#endif
    if (::bind(socket, reinterpret_cast<const sockaddr*>(&*endpoint), sizeof(*endpoint)) != 0) {
        return std::unexpected(socketFailure("debug listener bind"));
    }
    if (::listen(socket, std::max(backlog, 1)) != 0) {
        return std::unexpected(socketFailure("debug listener listen"));
    }
    sockaddr_in actual{};
#ifdef _WIN32
    i32 length = sizeof(actual);
#else
    socklen_t length = sizeof(actual);
#endif
    if (getsockname(socket, reinterpret_cast<sockaddr*>(&actual), &length) != 0) {
        return std::unexpected(socketFailure("debug listener endpoint query"));
    }
    result.localPort_ = ntohs(actual.sin_port);
    return result;
}

ProtocolResult<TcpConnection> TcpListener::accept() {
    const isize socket = socket_.load(std::memory_order_acquire);
    if (socket == -1) {
        return std::unexpected(ProtocolError{ProtocolStatus::InvalidState, "debug listener is closed", 0});
    }
    const NativeSocket accepted = ::accept(native(socket), nullptr, nullptr);
    if (accepted == kInvalidSocket) {
        return std::unexpected(socketFailure("debug listener accept"));
    }
    return TcpConnection(stored(accepted));
}

bool TcpListener::valid() const noexcept {
    return socket_.load(std::memory_order_acquire) != -1;
}

u16 TcpListener::localPort() const noexcept {
    return localPort_;
}

void TcpListener::close() noexcept {
    const isize socket = socket_.exchange(isize{-1}, std::memory_order_acq_rel);
    if (socket != -1) {
#ifdef _WIN32
        shutdown(native(socket), SD_BOTH);
#else
        shutdown(native(socket), SHUT_RDWR);
#endif
        closeSocket(native(socket));
        localPort_ = u16{0};
    }
}

} // namespace Lua::Debugger::Remote
