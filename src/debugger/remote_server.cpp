/**
 * @file remote_server.cpp
 * @brief Authenticated single-client Runtime Debug Server over YLDP.
 */

#include "debugger/remote_server.hpp"

#include <algorithm>
#include <chrono>
#include <deque>

namespace Lua::Debugger::Remote {

namespace {

constexpr u64 kBaseServerCapabilities = capabilityBit(ProtocolCapability::Breakpoints) |
                                    capabilityBit(ProtocolCapability::PauseContinue) |
                                    capabilityBit(ProtocolCapability::Stepping) |
                                    capabilityBit(ProtocolCapability::Stack) |
                                    capabilityBit(ProtocolCapability::Variables) |
                                    capabilityBit(ProtocolCapability::EvaluateReadOnly) |
                                    capabilityBit(ProtocolCapability::ExceptionInfo) |
                                    capabilityBit(ProtocolCapability::MultipleStates) |
                                    capabilityBit(ProtocolCapability::CoroutineThreads) |
                                    capabilityBit(ProtocolCapability::SourcePathMapping) |
                                    capabilityBit(ProtocolCapability::Reconnect) |
                                    capabilityBit(ProtocolCapability::GlobalPauseOnly) |
                                    capabilityBit(ProtocolCapability::AdvancedBreakpoints);

bool constantTimeEqual(StrView left, StrView right) noexcept {
    const usize extent = std::max(left.size(), right.size());
    u8 difference = static_cast<u8>(left.size() ^ right.size());
    for (usize index = 0; index < extent; ++index) {
        const u8 leftByte = index < left.size() ? static_cast<u8>(left[index]) : 0;
        const u8 rightByte = index < right.size() ? static_cast<u8>(right[index]) : 0;
        difference = static_cast<u8>(difference | static_cast<u8>(leftByte ^ rightByte));
    }
    return difference == 0;
}

ProtocolResult<Vec<u8>> emptyPayload() {
    return Vec<u8>{};
}

class FrameStream {
public:
    FrameStream(TcpConnection& connection, usize maxFrameBytes)
        : connection_(connection), decoder_(maxFrameBytes) {}

    ProtocolResult<ProtocolFrame> receive() {
        if (!pending_.empty()) {
            ProtocolFrame result = std::move(pending_.front());
            pending_.pop_front();
            return result;
        }
        for (;;) {
            auto bytes = connection_.receiveSome();
            if (!bytes) {
                return std::unexpected(bytes.error());
            }
            auto frames = decoder_.feed(std::span<const u8>(bytes->data(), bytes->size()));
            if (!frames) {
                return std::unexpected(frames.error());
            }
            for (ProtocolFrame& frame : *frames) {
                pending_.push_back(std::move(frame));
            }
            if (!pending_.empty()) {
                ProtocolFrame result = std::move(pending_.front());
                pending_.pop_front();
                return result;
            }
        }
    }

private:
    TcpConnection& connection_;
    ProtocolFrameDecoder decoder_;
    std::deque<ProtocolFrame> pending_;
};

} // namespace

class RuntimeDebugServer::Connection {
public:
    explicit Connection(TcpConnection socket, usize maxFrameBytes)
        : socket_(std::move(socket)), stream_(socket_, maxFrameBytes), maxFrameBytes_(maxFrameBytes) {}

    ProtocolResult<void> send(ProtocolFrame frame) {
        auto bytes = encodeProtocolFrame(frame, maxFrameBytes_);
        if (!bytes) {
            return std::unexpected(bytes.error());
        }
        std::lock_guard lock(sendMutex_);
        if (!socket_.valid()) {
            return std::unexpected(ProtocolError{ProtocolStatus::Terminated, "remote debug connection is closed", 0});
        }
        return socket_.sendAll(std::span<const u8>(bytes->data(), bytes->size()));
    }

    ProtocolResult<ProtocolFrame> receive() {
        return stream_.receive();
    }

    ProtocolResult<void> setTimeouts(u32 receiveTimeoutMs, u32 sendTimeoutMs) {
        return socket_.setTimeouts(receiveTimeoutMs, sendTimeoutMs);
    }

    void close() noexcept {
        std::lock_guard lock(sendMutex_);
        socket_.close();
    }

private:
    TcpConnection socket_;
    FrameStream stream_;
    usize maxFrameBytes_;
    std::mutex sendMutex_;
};

namespace {

class ServerEventSink final : public IDebugEventSink {
public:
    explicit ServerEventSink(WPtr<RuntimeDebugServer::Connection> connection) : connection_(std::move(connection)) {}

    void onDebugStateChanged(const DebugSessionSnapshot& snapshot) override {
        ProtocolFrame frame;
        frame.kind = ProtocolMessageKind::Event;
        if (snapshot.state == DebugSessionState::Suspended &&
            snapshot.pauseGeneration.value() != lastStoppedGeneration_.exchange(snapshot.pauseGeneration.value())) {
            frame.command = static_cast<ProtocolCommand>(ProtocolEvent::Stopped);
            auto payload = encodeStoppedEvent({snapshot.stopReason.value_or(DebugStopReason::Pause),
                                               snapshot.activeThread, snapshot.pauseGeneration});
            if (payload) {
                frame.payload = std::move(*payload);
                send(std::move(frame));
            }
        } else if (snapshot.state == DebugSessionState::Running &&
                   lastState_.exchange(snapshot.state) == DebugSessionState::ResumeRequested) {
            frame.command = static_cast<ProtocolCommand>(ProtocolEvent::Continued);
            auto payload = encodeThreadRequest(snapshot.activeThread);
            if (payload) {
                frame.payload = std::move(*payload);
                send(std::move(frame));
            }
        } else if (snapshot.state == DebugSessionState::Terminated &&
                   !terminated_.exchange(true, std::memory_order_acq_rel)) {
            frame.command = static_cast<ProtocolCommand>(ProtocolEvent::Terminated);
            auto payload = encodeTerminatedEvent(
                {snapshot.terminationReason.value_or(DebugTerminationReason::Completed), snapshot.lastError});
            if (payload) {
                frame.payload = std::move(*payload);
                send(std::move(frame));
            }
        }
        lastState_.store(snapshot.state, std::memory_order_release);
    }

    void onBreakpointChanged(const BreakpointBinding& binding) override {
        ProtocolFrame frame;
        frame.kind = ProtocolMessageKind::Event;
        frame.command = static_cast<ProtocolCommand>(ProtocolEvent::BreakpointChanged);
        frame.flags = binding.functionName ? 1U : 0U;
        auto payload = binding.functionName
                           ? encodeFunctionBreakpointBindings(std::span<const BreakpointBinding>(&binding, 1))
                           : encodeBreakpointBindings(std::span<const BreakpointBinding>(&binding, 1));
        if (payload) {
            frame.payload = std::move(*payload);
            send(std::move(frame));
        }
    }

    void onDebugOutput(StrView text, DebugOutputCategory category) override {
        ProtocolFrame frame;
        frame.kind = ProtocolMessageKind::Event;
        frame.command = static_cast<ProtocolCommand>(ProtocolEvent::Output);
        auto payload = encodeOutputEvent(text, category);
        if (payload) {
            frame.payload = std::move(*payload);
            send(std::move(frame));
        }
    }

    void onDebugExecutionUnitChanged(const DebugState& state, bool started) override {
        ProtocolFrame stateFrame;
        stateFrame.kind = ProtocolMessageKind::Event;
        stateFrame.command = static_cast<ProtocolCommand>(started ? ProtocolEvent::StateStarted
                                                                  : ProtocolEvent::StateExited);
        auto statePayload = encodeDebugStateEvent(state);
        if (statePayload) {
            stateFrame.payload = std::move(*statePayload);
            send(std::move(stateFrame));
        }

        ProtocolFrame threadFrame;
        threadFrame.kind = ProtocolMessageKind::Event;
        threadFrame.command = static_cast<ProtocolCommand>(started ? ProtocolEvent::ThreadStarted
                                                                   : ProtocolEvent::ThreadExited);
        const DebugThread thread{state.threadId, state.name, state.state};
        auto threadPayload = encodeThreads(std::span<const DebugThread>(&thread, 1));
        if (threadPayload) {
            threadFrame.payload = std::move(*threadPayload);
            send(std::move(threadFrame));
        }
    }

private:
    void send(ProtocolFrame frame) noexcept {
        if (Ptr<RuntimeDebugServer::Connection> connection = connection_.lock()) {
            (void)connection->send(std::move(frame));
        }
    }

    WPtr<RuntimeDebugServer::Connection> connection_;
    std::atomic<DebugSessionState> lastState_ = DebugSessionState::Detached;
    std::atomic<u64> lastStoppedGeneration_ = 0;
    std::atomic<bool> terminated_ = false;
};

ProtocolResult<void> sendResponse(const Ptr<RuntimeDebugServer::Connection>& connection, const ProtocolFrame& request,
                                  ProtocolResult<Vec<u8>> payload = emptyPayload()) {
    ProtocolFrame response;
    response.kind = ProtocolMessageKind::Response;
    response.command = request.command;
    response.requestId = request.requestId;
    if (!payload) {
        response.status = payload.error().status;
        auto error = encodeErrorMessage(payload.error().message);
        if (error) {
            response.payload = std::move(*error);
        }
    } else {
        response.payload = std::move(*payload);
    }
    return connection->send(std::move(response));
}

template <typename T>
ProtocolResult<void> sendDebugResult(const Ptr<RuntimeDebugServer::Connection>& connection,
                                     const ProtocolFrame& request, const DebugResult<T>& result,
                                     ProtocolResult<Vec<u8>> (*encoder)(const T&)) {
    if (!result) {
        ProtocolFrame response;
        response.kind = ProtocolMessageKind::Response;
        response.command = request.command;
        response.requestId = request.requestId;
        response.status = protocolStatus(result.error().code);
        auto message = encodeErrorMessage(result.error().message);
        if (message) {
            response.payload = std::move(*message);
        }
        return connection->send(std::move(response));
    }
    return sendResponse(connection, request, encoder(*result));
}

ProtocolResult<void> sendVoidDebugResult(const Ptr<RuntimeDebugServer::Connection>& connection,
                                         const ProtocolFrame& request, const DebugResult<void>& result) {
    if (result) {
        return sendResponse(connection, request);
    }
    ProtocolFrame response;
    response.kind = ProtocolMessageKind::Response;
    response.command = request.command;
    response.requestId = request.requestId;
    response.status = protocolStatus(result.error().code);
    auto message = encodeErrorMessage(result.error().message);
    if (message) {
        response.payload = std::move(*message);
    }
    return connection->send(std::move(response));
}

} // namespace

RuntimeDebugServer::RuntimeDebugServer(DebugController& runtime) : runtime_(runtime) {}

RuntimeDebugServer::~RuntimeDebugServer() {
    stop();
}

ProtocolResult<DebugServerEndpoint> RuntimeDebugServer::start(DebugServerConfig config) {
    std::lock_guard lifecycleLock(lifecycleMutex_);
    if (running_.load(std::memory_order_acquire)) {
        return std::unexpected(ProtocolError{ProtocolStatus::InvalidState, "debug server is already running", 0});
    }
    if (thread_.joinable()) {
        thread_.join();
    }
    std::lock_guard lock(mutex_);
    if (!config.enabled) {
        return std::unexpected(
            ProtocolError{ProtocolStatus::InvalidState, "remote debugging must be explicitly enabled", 0});
    }
    if (!config.allowNonLoopback && !isLoopbackAddress(config.bindAddress)) {
        return std::unexpected(ProtocolError{ProtocolStatus::Unauthorized,
                                             "non-loopback debug binding requires explicit authorization", 0});
    }
    if (config.authToken.size() < 16 || config.authToken.size() > kProtocolMaxStringBytes) {
        return std::unexpected(ProtocolError{ProtocolStatus::InvalidArgument,
                                             "remote debug token must contain at least 16 bounded bytes", 0});
    }
    if (config.maxConnections != 1) {
        return std::unexpected(
            ProtocolError{ProtocolStatus::InvalidArgument, "this runtime supports exactly one debug client", 0});
    }
    if (config.maxFrameBytes < kProtocolHeaderSize || config.maxFrameBytes > kProtocolMaxFrameBytes ||
        config.handshakeTimeoutMs == 0 || config.heartbeatIntervalMs == 0 ||
        config.idleTimeoutMs < config.heartbeatIntervalMs) {
        return std::unexpected(ProtocolError{ProtocolStatus::InvalidArgument, "invalid remote debug resource limits", 0});
    }
    auto listener = TcpListener::listen(config.bindAddress, config.port, 1);
    if (!listener) {
        return std::unexpected(listener.error());
    }
    config_ = std::move(config);
    listener_ = std::move(*listener);
    endpoint_ = {config_.bindAddress, listener_.localPort()};
    running_.store(true, std::memory_order_release);
    try {
        thread_ = std::thread([this]() { run(); });
    } catch (...) {
        running_.store(false, std::memory_order_release);
        listener_.close();
        endpoint_ = {};
        return std::unexpected(ProtocolError{ProtocolStatus::InternalError, "unable to start debug server thread", 0});
    }
    return endpoint_;
}

void RuntimeDebugServer::stop() noexcept {
    std::lock_guard lifecycleLock(lifecycleMutex_);
    running_.store(false, std::memory_order_release);
    Ptr<Connection> active;
    {
        std::lock_guard lock(mutex_);
        listener_.close();
        active = activeConnection_;
    }
    if (active) {
        active->close();
    }
    if (thread_.joinable()) {
        thread_.join();
    }
    std::lock_guard lock(mutex_);
    activeConnection_.reset();
    endpoint_ = {};
}

bool RuntimeDebugServer::running() const noexcept {
    return running_.load(std::memory_order_acquire);
}

DebugServerEndpoint RuntimeDebugServer::endpoint() const {
    std::lock_guard lock(mutex_);
    return endpoint_;
}

DebugServerStats RuntimeDebugServer::stats() const noexcept {
    return {acceptedConnections_.load(std::memory_order_relaxed),
            authenticatedSessions_.load(std::memory_order_relaxed),
            authenticationFailures_.load(std::memory_order_relaxed),
            protocolFailures_.load(std::memory_order_relaxed),
            idleDisconnects_.load(std::memory_order_relaxed)};
}

void RuntimeDebugServer::run() noexcept {
    while (running_.load(std::memory_order_acquire)) {
        ProtocolResult<TcpConnection> accepted = std::unexpected(ProtocolError{});
        try {
            accepted = listener_.accept();
        } catch (...) {
            protocolFailures_.fetch_add(1, std::memory_order_relaxed);
            break;
        }
        if (!accepted) {
            if (running_.load(std::memory_order_acquire)) {
                protocolFailures_.fetch_add(1, std::memory_order_relaxed);
            }
            break;
        }
        Ptr<Connection> connection;
        try {
            acceptedConnections_.fetch_add(1, std::memory_order_relaxed);
            connection = makePtr<Connection>(std::move(*accepted), config_.maxFrameBytes);
            {
                std::lock_guard lock(mutex_);
                activeConnection_ = connection;
            }
            serve(connection);
        } catch (...) {
            protocolFailures_.fetch_add(1, std::memory_order_relaxed);
        }
        if (connection) {
            connection->close();
        }
        {
            std::lock_guard lock(mutex_);
            if (activeConnection_ == connection) {
                activeConnection_.reset();
            }
        }
    }
    running_.store(false, std::memory_order_release);
}

void RuntimeDebugServer::serve(const Ptr<Connection>& connection) {
    if (!connection->setTimeouts(config_.handshakeTimeoutMs, config_.handshakeTimeoutMs)) {
        return;
    }
    auto helloFrame = connection->receive();
    if (!helloFrame || helloFrame->kind != ProtocolMessageKind::Hello || helloFrame->command != ProtocolCommand::None ||
        helloFrame->requestId == 0) {
        protocolFailures_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    auto hello = decodeHello(std::span<const u8>(helloFrame->payload.data(), helloFrame->payload.size()));
    if (!hello) {
        (void)sendResponse(connection, *helloFrame, std::unexpected(hello.error()));
        protocolFailures_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    if (!constantTimeEqual(hello->authToken, config_.authToken)) {
        ProtocolFrame rejected;
        rejected.kind = ProtocolMessageKind::HelloAck;
        rejected.requestId = helloFrame->requestId;
        rejected.status = ProtocolStatus::Unauthorized;
        auto message = encodeErrorMessage("remote debug authentication failed");
        if (message) {
            rejected.payload = std::move(*message);
        }
        (void)connection->send(std::move(rejected));
        authenticationFailures_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    u64 availableCapabilities = kBaseServerCapabilities;
    if (config_.allowVariableWrite) {
        availableCapabilities |= capabilityBit(ProtocolCapability::VariableWrite);
    }
    if (config_.allowSideEffectEvaluation) {
        availableCapabilities |= capabilityBit(ProtocolCapability::SideEffectEvaluation);
    }
    auto negotiated = negotiateHello(*hello, availableCapabilities, authenticatedSessions_.load() + 1,
                                     config_.serverVersion);
    if (!negotiated) {
        ProtocolFrame rejected;
        rejected.kind = ProtocolMessageKind::HelloAck;
        rejected.requestId = helloFrame->requestId;
        rejected.status = negotiated.error().status;
        auto message = encodeErrorMessage(negotiated.error().message);
        if (message) {
            rejected.payload = std::move(*message);
        }
        (void)connection->send(std::move(rejected));
        return;
    }

    auto writePolicy = runtime_.configureWritePolicy(
        DebugWritePolicy{config_.allowVariableWrite, config_.allowSideEffectEvaluation});
    if (!writePolicy) {
        ProtocolFrame rejected;
        rejected.kind = ProtocolMessageKind::HelloAck;
        rejected.requestId = helloFrame->requestId;
        rejected.status = ProtocolStatus::InvalidState;
        auto message = encodeErrorMessage(writePolicy.error().message);
        if (message) {
            rejected.payload = std::move(*message);
        }
        (void)connection->send(std::move(rejected));
        return;
    }
    struct ResetWritePolicy {
        DebugController& runtime;
        ~ResetWritePolicy() {
            (void)runtime.configureWritePolicy({});
        }
    } resetWritePolicy{runtime_};

    Ptr<ServerEventSink> sink = makePtr<ServerEventSink>(connection);
    auto attached = runtime_.attachSession(sink, config_.disconnectAction);
    if (!attached) {
        ProtocolFrame rejected;
        rejected.kind = ProtocolMessageKind::HelloAck;
        rejected.requestId = helloFrame->requestId;
        rejected.status = ProtocolStatus::Busy;
        auto message = encodeErrorMessage(attached.error().message);
        if (message) {
            rejected.payload = std::move(*message);
        }
        (void)connection->send(std::move(rejected));
        return;
    }
    DebugSession session = std::move(*attached);
    if (auto configured = runtime_.configurationDone(); !configured) {
        ProtocolFrame rejected;
        rejected.kind = ProtocolMessageKind::HelloAck;
        rejected.requestId = helloFrame->requestId;
        rejected.status = ProtocolStatus::InvalidState;
        auto message = encodeErrorMessage(configured.error().message);
        if (message) {
            rejected.payload = std::move(*message);
        }
        (void)connection->send(std::move(rejected));
        session.disconnect(config_.disconnectAction);
        return;
    }
    if (!hello->stateSelector.empty()) {
        auto states = runtime_.states();
        StateId selected;
        if (states) {
            for (const DebugState& state : *states) {
                if (state.name == hello->stateSelector || state.label == hello->stateSelector) {
                    selected = state.id;
                    break;
                }
            }
        }
        auto selection = selected.valid()
                             ? runtime_.selectState(selected)
                             : DebugResult<void>{std::unexpected(
                                   DebugError{DebugErrorCode::InvalidReference, "requested debug state was not found"})};
        if (!selection) {
            ProtocolFrame rejected;
            rejected.kind = ProtocolMessageKind::HelloAck;
            rejected.requestId = helloFrame->requestId;
            rejected.status = ProtocolStatus::InvalidArgument;
            auto message = encodeErrorMessage(selection.error().message);
            if (message) {
                rejected.payload = std::move(*message);
            }
            (void)connection->send(std::move(rejected));
            session.disconnect(config_.disconnectAction);
            return;
        }
    }

    negotiated->heartbeatIntervalMs = config_.heartbeatIntervalMs;
    auto ackPayload = encodeHelloAck(*negotiated);
    if (!ackPayload) {
        return;
    }
    ProtocolFrame ack;
    ack.kind = ProtocolMessageKind::HelloAck;
    ack.requestId = helloFrame->requestId;
    ack.payload = std::move(*ackPayload);
    if (!connection->send(std::move(ack))) {
        session.disconnect(config_.disconnectAction);
        return;
    }
    authenticatedSessions_.fetch_add(1, std::memory_order_relaxed);
    (void)connection->setTimeouts(config_.heartbeatIntervalMs, config_.handshakeTimeoutMs);

    HashSet<u64> requestIds;
    u64 pingId = 1;
    u32 idleMs = 0;
    bool keepServing = true;
    while (keepServing && running_.load(std::memory_order_acquire)) {
        auto frame = connection->receive();
        if (!frame) {
            if (frame.error().status == ProtocolStatus::Timeout) {
                idleMs += config_.heartbeatIntervalMs;
                if (idleMs >= config_.idleTimeoutMs) {
                    idleDisconnects_.fetch_add(1, std::memory_order_relaxed);
                    break;
                }
                ProtocolFrame ping;
                ping.kind = ProtocolMessageKind::Ping;
                ping.requestId = pingId++;
                if (!connection->send(std::move(ping))) {
                    break;
                }
                continue;
            }
            break;
        }
        idleMs = 0;
        if (frame->kind == ProtocolMessageKind::Ping) {
            ProtocolFrame pong;
            pong.kind = ProtocolMessageKind::Pong;
            pong.requestId = frame->requestId;
            (void)connection->send(std::move(pong));
            continue;
        }
        if (frame->kind == ProtocolMessageKind::Pong) {
            continue;
        }
        if (frame->kind == ProtocolMessageKind::Goodbye) {
            break;
        }
        if (frame->kind != ProtocolMessageKind::Request || frame->requestId == 0) {
            protocolFailures_.fetch_add(1, std::memory_order_relaxed);
            break;
        }
        if (!requestIds.insert(frame->requestId).second) {
            (void)sendResponse(connection, *frame,
                               std::unexpected(ProtocolError{ProtocolStatus::ProtocolError,
                                                             "duplicate remote request ID", 0}));
            continue;
        }
        if (requestIds.size() > kProtocolMaxCollectionItems) {
            requestIds.clear();
            requestIds.insert(frame->requestId);
        }

        const auto payload = std::span<const u8>(frame->payload.data(), frame->payload.size());
        switch (frame->command) {
        case ProtocolCommand::SetBreakpoints:
        case ProtocolCommand::SetAdvancedBreakpoints: {
            auto request = frame->command == ProtocolCommand::SetAdvancedBreakpoints
                               ? decodeAdvancedBreakpointRequest(payload)
                               : decodeBreakpointRequest(payload);
            if (!request) {
                (void)sendResponse(connection, *frame, std::unexpected(request.error()));
                break;
            }
            const SourceId source = runtime_.registerFilePath(request->sourcePath);
            auto result = runtime_.setBreakpoints(source, request->breakpoints);
            if (!result) {
                (void)sendVoidDebugResult(connection, *frame, std::unexpected(result.error()));
            } else {
                (void)sendResponse(connection, *frame, encodeBreakpointBindings(*result));
            }
            break;
        }
        case ProtocolCommand::SetFunctionBreakpoints: {
            auto request = decodeFunctionBreakpointRequest(payload);
            if (!request) {
                (void)sendResponse(connection, *frame, std::unexpected(request.error()));
                break;
            }
            auto result = runtime_.setFunctionBreakpoints(*request);
            if (!result) {
                (void)sendVoidDebugResult(connection, *frame, std::unexpected(result.error()));
            } else {
                (void)sendResponse(connection, *frame, encodeFunctionBreakpointBindings(*result));
            }
            break;
        }
        case ProtocolCommand::Pause:
        case ProtocolCommand::Continue:
        case ProtocolCommand::Next:
        case ProtocolCommand::StepIn:
        case ProtocolCommand::StepOut:
        case ProtocolCommand::ExceptionInfo: {
            auto thread = decodeThreadRequest(payload);
            if (!thread) {
                (void)sendResponse(connection, *frame, std::unexpected(thread.error()));
                break;
            }
            if (frame->command == ProtocolCommand::Pause) {
                (void)sendVoidDebugResult(connection, *frame, runtime_.pause(*thread));
            } else if (frame->command == ProtocolCommand::Continue) {
                (void)sendVoidDebugResult(connection, *frame, runtime_.continueExecution(*thread));
            } else if (frame->command == ProtocolCommand::Next) {
                (void)sendVoidDebugResult(connection, *frame, runtime_.stepExecution(*thread, DebugStepMode::Over));
            } else if (frame->command == ProtocolCommand::StepIn) {
                (void)sendVoidDebugResult(connection, *frame, runtime_.stepExecution(*thread, DebugStepMode::In));
            } else if (frame->command == ProtocolCommand::StepOut) {
                (void)sendVoidDebugResult(connection, *frame, runtime_.stepExecution(*thread, DebugStepMode::Out));
            } else {
                auto result = runtime_.exceptionInfo(*thread);
                if (!result) {
                    (void)sendVoidDebugResult(connection, *frame, std::unexpected(result.error()));
                } else {
                    (void)sendResponse(connection, *frame, encodeExceptionInfo(*result));
                }
            }
            break;
        }
        case ProtocolCommand::Threads: {
            auto result = runtime_.threads();
            if (!result) {
                (void)sendVoidDebugResult(connection, *frame, std::unexpected(result.error()));
            } else {
                (void)sendResponse(connection, *frame, encodeThreads(*result));
            }
            break;
        }
        case ProtocolCommand::States: {
            auto result = runtime_.states();
            if (!result) {
                (void)sendVoidDebugResult(connection, *frame, std::unexpected(result.error()));
            } else {
                (void)sendResponse(connection, *frame, encodeStates(*result));
            }
            break;
        }
        case ProtocolCommand::SelectState: {
            auto state = decodeStateRequest(payload);
            if (!state) {
                (void)sendResponse(connection, *frame, std::unexpected(state.error()));
            } else {
                (void)sendVoidDebugResult(connection, *frame, runtime_.selectState(*state));
            }
            break;
        }
        case ProtocolCommand::StackTrace: {
            auto request = decodeStackTraceRequest(payload);
            if (!request) {
                (void)sendResponse(connection, *frame, std::unexpected(request.error()));
                break;
            }
            auto result = runtime_.stackTrace(request->thread, request->startFrame, request->levels);
            if (!result) {
                (void)sendVoidDebugResult(connection, *frame, std::unexpected(result.error()));
                break;
            }
            Vec<RemoteStackFrame> remote;
            remote.reserve(result->size());
            for (DebugStackFrame& stackFrame : *result) {
                Str sourceName;
                bool sourceIsFile = false;
                if (auto source = runtime_.source(stackFrame.location.sourceId)) {
                    sourceName = source->source.displayName;
                    sourceIsFile = source->source.kind == SourceKind::File;
                }
                remote.push_back({std::move(stackFrame), std::move(sourceName), sourceIsFile});
            }
            (void)sendResponse(connection, *frame, encodeStackFrames(remote));
            break;
        }
        case ProtocolCommand::Scopes: {
            auto request = decodeFrameRequest(payload);
            if (!request) {
                (void)sendResponse(connection, *frame, std::unexpected(request.error()));
                break;
            }
            auto result = runtime_.scopes(*request);
            if (!result) {
                (void)sendVoidDebugResult(connection, *frame, std::unexpected(result.error()));
            } else {
                (void)sendResponse(connection, *frame, encodeScopes(*result));
            }
            break;
        }
        case ProtocolCommand::Variables: {
            auto request = decodeVariablesRequest(payload);
            if (!request) {
                (void)sendResponse(connection, *frame, std::unexpected(request.error()));
                break;
            }
            auto result = runtime_.variables(request->reference, request->start, request->count, request->filter);
            if (!result) {
                (void)sendVoidDebugResult(connection, *frame, std::unexpected(result.error()));
            } else {
                (void)sendResponse(connection, *frame, encodeVariables(*result));
            }
            break;
        }
        case ProtocolCommand::Evaluate:
        case ProtocolCommand::EvaluateSideEffects: {
            if (frame->command == ProtocolCommand::EvaluateSideEffects && !config_.allowSideEffectEvaluation) {
                (void)sendResponse(connection, *frame,
                                   std::unexpected(ProtocolError{ProtocolStatus::Unauthorized,
                                                                 "remote side-effect evaluation is disabled", 0}));
                break;
            }
            auto request = decodeEvaluateRequest(payload);
            if (!request) {
                (void)sendResponse(connection, *frame, std::unexpected(request.error()));
                break;
            }
            auto result = frame->command == ProtocolCommand::EvaluateSideEffects
                              ? runtime_.evaluateWithSideEffects(request->frame, request->expression)
                              : runtime_.evaluate(request->frame, request->expression);
            if (!result) {
                (void)sendVoidDebugResult(connection, *frame, std::unexpected(result.error()));
            } else {
                (void)sendResponse(connection, *frame, encodeVariable(*result));
            }
            break;
        }
        case ProtocolCommand::SetVariable: {
            if (!config_.allowVariableWrite) {
                (void)sendResponse(connection, *frame,
                                   std::unexpected(ProtocolError{ProtocolStatus::Unauthorized,
                                                                 "remote variable writes are disabled", 0}));
                break;
            }
            auto request = decodeSetVariableRequest(payload);
            if (!request) {
                (void)sendResponse(connection, *frame, std::unexpected(request.error()));
                break;
            }
            auto result = runtime_.setVariable(request->reference, request->name, request->valueExpression);
            if (!result) {
                (void)sendVoidDebugResult(connection, *frame, std::unexpected(result.error()));
            } else {
                (void)sendResponse(connection, *frame, encodeVariable(*result));
            }
            break;
        }
        case ProtocolCommand::SetExceptionBreakpoints: {
            auto enabled = decodeBooleanRequest(payload);
            if (!enabled) {
                (void)sendResponse(connection, *frame, std::unexpected(enabled.error()));
            } else {
                (void)sendVoidDebugResult(connection, *frame, runtime_.setExceptionBreakpoints(*enabled));
            }
            break;
        }
        case ProtocolCommand::Detach: {
            auto terminate = decodeBooleanRequest(payload);
            if (!terminate) {
                (void)sendResponse(connection, *frame, std::unexpected(terminate.error()));
                break;
            }
            (void)sendResponse(connection, *frame);
            session.disconnect(*terminate ? DisconnectAction::TerminateExecution : DisconnectAction::ContinueExecution);
            keepServing = false;
            break;
        }
        default:
            (void)sendResponse(connection, *frame,
                               std::unexpected(ProtocolError{ProtocolStatus::NotSupported,
                                                             "remote debug command is not supported", 0}));
            break;
        }
    }
    if (session.attached()) {
        session.disconnect(config_.disconnectAction);
    }
}

} // namespace Lua::Debugger::Remote
