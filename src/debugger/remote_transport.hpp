#pragma once

/**
 * @file remote_transport.hpp
 * @brief Small cross-platform TCP transport used only by the opt-in remote debugger.
 */

#include "debugger/remote_protocol.hpp"

#include <atomic>
#include <span>

namespace Lua::Debugger::Remote {

class TcpConnection {
public:
    TcpConnection() noexcept = default;
    ~TcpConnection();

    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;
    TcpConnection(TcpConnection&& other) noexcept;
    TcpConnection& operator=(TcpConnection&& other) noexcept;

    [[nodiscard]] static ProtocolResult<TcpConnection> connect(StrView address, u16 port, u32 timeoutMs);
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] ProtocolResult<void> sendAll(std::span<const u8> bytes);
    [[nodiscard]] ProtocolResult<Vec<u8>> receiveSome(usize maxBytes = 64U * 1024U);
    [[nodiscard]] ProtocolResult<void> setTimeouts(u32 receiveTimeoutMs, u32 sendTimeoutMs);
    void close() noexcept;

private:
    friend class TcpListener;
    explicit TcpConnection(isize socket) noexcept : socket_(socket) {}
    std::atomic<isize> socket_{-1};
};

class TcpListener {
public:
    TcpListener() noexcept = default;
    ~TcpListener();

    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;
    TcpListener(TcpListener&& other) noexcept;
    TcpListener& operator=(TcpListener&& other) noexcept;

    [[nodiscard]] static ProtocolResult<TcpListener> listen(StrView address, u16 port, i32 backlog = 1);
    [[nodiscard]] ProtocolResult<TcpConnection> accept();
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] u16 localPort() const noexcept;
    void close() noexcept;

private:
    std::atomic<isize> socket_{-1};
    u16 localPort_ = 0;
};

[[nodiscard]] bool isLoopbackAddress(StrView address) noexcept;

} // namespace Lua::Debugger::Remote
