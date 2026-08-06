#pragma once

/**
 * @file remote_server.hpp
 * @brief Opt-in authenticated YLDP server for a runtime DebugController.
 */

#include "debugger/remote_messages.hpp"
#include "debugger/remote_transport.hpp"

#include <atomic>
#include <mutex>
#include <thread>

namespace Lua::Debugger::Remote {

struct DebugServerConfig {
    bool enabled = false;
    Str bindAddress = "127.0.0.1";
    u16 port = 0;
    Str authToken;
    bool allowNonLoopback = false;
    usize maxConnections = 1;
    usize maxFrameBytes = kProtocolMaxFrameBytes;
    u32 handshakeTimeoutMs = kProtocolHandshakeTimeoutMs;
    u32 heartbeatIntervalMs = kProtocolHeartbeatIntervalMs;
    u32 idleTimeoutMs = kProtocolIdleTimeoutMs;
    DisconnectAction disconnectAction = DisconnectAction::ContinueExecution;
    Str serverVersion = "0.1.0";
};

struct DebugServerEndpoint {
    Str address;
    u16 port = 0;
};

struct DebugServerStats {
    u64 acceptedConnections = 0;
    u64 authenticatedSessions = 0;
    u64 authenticationFailures = 0;
    u64 protocolFailures = 0;
    u64 idleDisconnects = 0;
};

class RuntimeDebugServer {
public:
    class Connection;

    explicit RuntimeDebugServer(DebugController& runtime);
    ~RuntimeDebugServer();

    RuntimeDebugServer(const RuntimeDebugServer&) = delete;
    RuntimeDebugServer& operator=(const RuntimeDebugServer&) = delete;

    [[nodiscard]] ProtocolResult<DebugServerEndpoint> start(DebugServerConfig config);
    void stop() noexcept;
    [[nodiscard]] bool running() const noexcept;
    [[nodiscard]] DebugServerEndpoint endpoint() const;
    [[nodiscard]] DebugServerStats stats() const noexcept;

private:
    void run() noexcept;
    void serve(const Ptr<Connection>& connection) noexcept;

    DebugController& runtime_;
    mutable std::mutex mutex_;
    DebugServerConfig config_;
    DebugServerEndpoint endpoint_;
    TcpListener listener_;
    Ptr<Connection> activeConnection_;
    std::thread thread_;
    std::atomic<bool> running_ = false;
    std::atomic<u64> acceptedConnections_ = 0;
    std::atomic<u64> authenticatedSessions_ = 0;
    std::atomic<u64> authenticationFailures_ = 0;
    std::atomic<u64> protocolFailures_ = 0;
    std::atomic<u64> idleDisconnects_ = 0;
};

} // namespace Lua::Debugger::Remote
