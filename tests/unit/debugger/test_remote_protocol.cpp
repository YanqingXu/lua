/**
 * @file test_remote_protocol.cpp
 * @brief YLDP framing, compatibility, limits, and malformed-input tests.
 */

#include "../framework/test_framework.hpp"

#include "debugger/remote_protocol.hpp"
#include "debugger/remote_messages.hpp"
#include "debugger/remote_server.hpp"
#include "debugger/remote_transport.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"

#include <array>
#include <chrono>
#include <deque>
#include <span>
#include <thread>

using namespace Lua;
using namespace Lua::Debugger;
using namespace Lua::Debugger::Remote;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Debugger Remote Protocol";

std::span<const u8> bytesOf(const Vec<u8>& bytes) {
    return std::span<const u8>(bytes.data(), bytes.size());
}

ProtocolResult<void> sendFrame(TcpConnection& connection, ProtocolFrame frame) {
    auto bytes = encodeProtocolFrame(frame);
    if (!bytes) {
        return std::unexpected(bytes.error());
    }
    return connection.sendAll(bytesOf(*bytes));
}

class ProtocolFrameReader {
public:
    ProtocolResult<ProtocolFrame> receive(TcpConnection& connection) {
        for (;;) {
            if (!pending_.empty()) {
                ProtocolFrame frame = std::move(pending_.front());
                pending_.pop_front();
                return frame;
            }
            auto bytes = connection.receiveSome();
            if (!bytes) {
                return std::unexpected(bytes.error());
            }
            auto frames = decoder_.feed(bytesOf(*bytes));
            if (!frames) {
                return std::unexpected(frames.error());
            }
            for (ProtocolFrame& frame : *frames) {
                pending_.push_back(std::move(frame));
            }
        }
    }

private:
    ProtocolFrameDecoder decoder_;
    std::deque<ProtocolFrame> pending_;
};

ProtocolResult<ProtocolFrame> exchangeRequest(TcpConnection& connection, ProtocolFrameReader& reader,
                                              ProtocolCommand command, u64 requestId, Vec<u8> payload = {}) {
    ProtocolFrame request;
    request.kind = ProtocolMessageKind::Request;
    request.command = command;
    request.requestId = requestId;
    request.payload = std::move(payload);
    if (auto sent = sendFrame(connection, std::move(request)); !sent) {
        return std::unexpected(sent.error());
    }
    return reader.receive(connection);
}

ProtocolResult<ProtocolFrame> exchangeHello(TcpConnection& connection, ProtocolFrameReader& reader, Str token,
                                            u64 requestId = 1, u16 minimumMajor = kProtocolVersionMajor,
                                            u16 maximumMajor = kProtocolVersionMajor, Str stateSelector = {}) {
    ProtocolHello hello;
    hello.minimumMajor = minimumMajor;
    hello.maximumMajor = maximumMajor;
    hello.clientName = "lua-cpp contract client";
    hello.clientVersion = "1.0";
    hello.authToken = std::move(token);
    hello.requestedCapabilities =
        capabilityBit(ProtocolCapability::Breakpoints) | capabilityBit(ProtocolCapability::MultipleStates) |
        capabilityBit(ProtocolCapability::VariableWrite) | capabilityBit(ProtocolCapability::SideEffectEvaluation);
    hello.stateSelector = std::move(stateSelector);
    auto payload = encodeHello(hello);
    if (!payload) {
        return std::unexpected(payload.error());
    }
    ProtocolFrame frame;
    frame.kind = ProtocolMessageKind::Hello;
    frame.requestId = requestId;
    frame.payload = std::move(*payload);
    if (auto sent = sendFrame(connection, std::move(frame)); !sent) {
        return std::unexpected(sent.error());
    }
    return reader.receive(connection);
}

bool waitForServerStats(const RuntimeDebugServer& server, u64 acceptedConnections, u64 authenticatedSessions,
                        u64 authenticationFailures, u64 protocolFailures) {
    for (usize attempt = 0; attempt < 200; ++attempt) {
        const DebugServerStats stats = server.stats();
        if (stats.acceptedConnections >= acceptedConnections && stats.authenticatedSessions >= authenticatedSessions &&
            stats.authenticationFailures >= authenticationFailures && stats.protocolFailures >= protocolFailures) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

} // namespace

void testRemoteProtocolPrimitives(TestSuite& suite) {
    ProtocolWriter writer;
    writer.writeU8(0x7fU);
    writer.writeU16(0x1234U);
    writer.writeU32(0x89abcdefU);
    writer.writeU64(0x0123456789abcdefULL);
    writer.writeI64(-42);
    writer.writeBool(true);
    writer.writeString("YanLua");
    const std::array raw{u8{1}, u8{2}, u8{3}};
    writer.writeBytes(raw);
    auto payload = std::move(writer).finish();
    ASSERT_TRUE(suite, payload.has_value(), "Bounded protocol writer encodes primitive values");

    ProtocolReader reader(payload ? bytesOf(*payload) : std::span<const u8>{});
    const auto value8 = reader.readU8();
    const auto value16 = reader.readU16();
    const auto value32 = reader.readU32();
    const auto value64 = reader.readU64();
    const auto signedValue = reader.readI64();
    const auto boolean = reader.readBool();
    const auto string = reader.readString();
    const auto bytes = reader.readBytes();
    ASSERT_TRUE(suite,
                value8 && *value8 == 0x7fU && value16 && *value16 == 0x1234U && value32 && *value32 == 0x89abcdefU &&
                    value64 && *value64 == 0x0123456789abcdefULL,
                "Protocol integers use deterministic network byte order");
    ASSERT_TRUE(suite, signedValue && *signedValue == -42 && boolean && *boolean && string && *string == "YanLua",
                "Protocol signed, boolean, and string values round trip");
    ASSERT_TRUE(suite, bytes && *bytes == Vec<u8>(raw.begin(), raw.end()) && reader.finish().has_value(),
                "Protocol byte vectors consume the payload exactly");

    ProtocolWriter tooSmall(4);
    tooSmall.writeString("too large");
    const auto rejected = std::move(tooSmall).finish();
    ASSERT_TRUE(suite, !rejected && rejected.error().status == ProtocolStatus::ResourceLimit,
                "Protocol writer rejects payload growth beyond its configured limit");

    const std::array invalidBool{u8{2}};
    ProtocolReader invalidReader(invalidBool);
    ASSERT_FALSE(suite, invalidReader.readBool(), "Protocol reader rejects non-canonical booleans");
}

void testRemoteProtocolFrames(TestSuite& suite) {
    ProtocolFrame original;
    original.kind = ProtocolMessageKind::Request;
    original.command = ProtocolCommand::Evaluate;
    original.requestId = 0x1020304050607080ULL;
    original.payload = {0, 1, 2, 3, 0xff};
    auto encoded = encodeProtocolFrame(original);
    ASSERT_TRUE(suite, encoded.has_value(), "YLDP frame encoder accepts a bounded request");
    const auto decoded = encoded ? decodeProtocolFrame(bytesOf(*encoded))
                                 : ProtocolResult<ProtocolFrame>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decoded && decoded->major == kProtocolVersionMajor && decoded->kind == ProtocolMessageKind::Request &&
                    decoded->command == ProtocolCommand::Evaluate && decoded->requestId == original.requestId &&
                    decoded->payload == original.payload,
                "YLDP fixed header and opaque payload round trip without native-layout data");

    ProtocolFrameDecoder fragmented;
    Vec<ProtocolFrame> fragments;
    if (encoded) {
        for (const u8 byte : *encoded) {
            const std::array one{byte};
            auto batch = fragmented.feed(one);
            if (batch) {
                for (ProtocolFrame& frame : *batch) {
                    fragments.push_back(std::move(frame));
                }
            }
        }
    }
    ASSERT_TRUE(
        suite, fragments.size() == 1 && fragments[0].requestId == original.requestId && fragmented.finish().has_value(),
        "Incremental decoder accepts a frame fragmented at every byte boundary");

    ProtocolFrame second = original;
    second.requestId = 2;
    second.command = ProtocolCommand::Threads;
    const auto secondBytes = encodeProtocolFrame(second);
    Vec<u8> merged = encoded.value_or(Vec<u8>{});
    if (secondBytes) {
        merged.insert(merged.end(), secondBytes->begin(), secondBytes->end());
    }
    ProtocolFrameDecoder mergedDecoder;
    const auto mergedFrames = mergedDecoder.feed(bytesOf(merged));
    ASSERT_TRUE(suite, mergedFrames && mergedFrames->size() == 2 && (*mergedFrames)[1].requestId == 2,
                "Incremental decoder separates coalesced YLDP frames");
}

void testRemoteProtocolMalformedInputs(TestSuite& suite) {
    ProtocolFrame frame;
    frame.requestId = 1;
    const auto encodedResult = encodeProtocolFrame(frame);
    Vec<u8> encoded = encodedResult.value_or(Vec<u8>{});

    Vec<u8> badMagic = encoded;
    if (badMagic.size() > 7) {
        badMagic[7] ^= 0xffU;
    }
    ASSERT_FALSE(suite, decodeProtocolFrame(bytesOf(badMagic)), "YLDP rejects a mismatched magic value");

    Vec<u8> badReserved = encoded;
    if (badReserved.size() > 27) {
        badReserved[27] = 1;
    }
    ASSERT_FALSE(suite, decodeProtocolFrame(bytesOf(badReserved)), "YLDP rejects non-zero reserved header bits");

    ProtocolFrameDecoder shortLength;
    const std::array impossibleLength{u8{0}, u8{0}, u8{0}, u8{1}};
    ASSERT_FALSE(suite, shortLength.feed(impossibleLength), "YLDP rejects lengths smaller than the fixed header");

    ProtocolFrameDecoder oversized(64);
    const std::array largeLength{u8{0}, u8{0}, u8{1}, u8{0}};
    const auto large = oversized.feed(largeLength);
    ASSERT_TRUE(suite, !large && large.error().status == ProtocolStatus::ResourceLimit,
                "YLDP rejects an oversized frame before allocating its declared body");

    ProtocolFrameDecoder earlyEof;
    const usize partialSize = encoded.size() > 2 ? encoded.size() - 2 : 0;
    const auto partial = earlyEof.feed(std::span<const u8>(encoded.data(), partialSize));
    ASSERT_TRUE(suite, partial.has_value() && !earlyEof.finish(), "YLDP reports bounded early EOF for a partial frame");
}

void testRemoteProtocolHandshake(TestSuite& suite) {
    ProtocolHello hello;
    hello.clientName = "YanLua Debug Adapter";
    hello.clientVersion = "0.1.0";
    hello.authToken = "secret-never-echoed";
    hello.requestedCapabilities = capabilityBit(ProtocolCapability::Breakpoints) |
                                  capabilityBit(ProtocolCapability::Stepping) |
                                  capabilityBit(ProtocolCapability::MultipleStates);
    hello.stateSelector = "game";
    const auto payload = encodeHello(hello);
    const auto decoded =
        payload ? decodeHello(bytesOf(*payload)) : ProtocolResult<ProtocolHello>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decoded && decoded->clientName == hello.clientName && decoded->authToken == hello.authToken &&
                    decoded->stateSelector == "game",
                "YLDP hello round trips version, authentication, capability, and state selection fields");

    const u64 available =
        capabilityBit(ProtocolCapability::Breakpoints) | capabilityBit(ProtocolCapability::GlobalPauseOnly);
    const auto negotiated = decoded ? negotiateHello(*decoded, available, 73, "1.2.3")
                                    : ProtocolResult<ProtocolHelloAck>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                negotiated && negotiated->sessionId == 73 &&
                    negotiated->capabilities == capabilityBit(ProtocolCapability::Breakpoints),
                "Handshake selects only the intersection of requested and available capabilities");
    const auto ackPayload =
        negotiated ? encodeHelloAck(*negotiated) : ProtocolResult<Vec<u8>>(std::unexpected(ProtocolError{}));
    const auto ack = ackPayload ? decodeHelloAck(bytesOf(*ackPayload))
                                : ProtocolResult<ProtocolHelloAck>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite, ack && ack->serverVersion == "1.2.3" && ack->serverName == "YanLua Runtime",
                "Handshake acknowledgement carries stable server metadata");
    ASSERT_TRUE(suite,
                !ackPayload ||
                    Str(reinterpret_cast<const char*>(ackPayload->data()), ackPayload->size()).find(hello.authToken) ==
                        Str::npos,
                "Authentication token is never echoed by the server handshake");

    ProtocolHello incompatible = hello;
    incompatible.minimumMajor = 2;
    incompatible.maximumMajor = 2;
    const auto mismatch = negotiateHello(incompatible, available, 1, "1.0.0");
    ASSERT_TRUE(suite, !mismatch && mismatch.error().status == ProtocolStatus::VersionMismatch,
                "Incompatible protocol major versions fail with a stable status");
}

void testRemoteAdvancedBreakpointMessages(TestSuite& suite) {
    RemoteBreakpointRequest request;
    request.sourcePath = "/srv/game/advanced.lua";
    request.breakpoints.push_back(SourceBreakpoint{17, "enabled", "3", "value={value}"});
    auto encoded = encodeAdvancedBreakpointRequest(request);
    auto decoded = encoded ? decodeAdvancedBreakpointRequest(bytesOf(*encoded))
                           : ProtocolResult<RemoteBreakpointRequest>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decoded && decoded->sourcePath == request.sourcePath && decoded->breakpoints.size() == 1 &&
                    decoded->breakpoints[0].condition == "enabled" && decoded->breakpoints[0].hitCondition == "3" &&
                    decoded->breakpoints[0].logMessage == "value={value}",
                "Advanced source breakpoint fields round trip over the versioned YLDP codec");

    const std::array functions{FunctionBreakpoint{"worker", "ready", "2"}};
    auto functionPayload = encodeFunctionBreakpointRequest(functions);
    auto decodedFunctions = functionPayload ? decodeFunctionBreakpointRequest(bytesOf(*functionPayload))
                                            : ProtocolResult<Vec<FunctionBreakpoint>>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decodedFunctions && decodedFunctions->size() == 1 && decodedFunctions->front().name == "worker" &&
                    decodedFunctions->front().condition == "ready" && decodedFunctions->front().hitCondition == "2",
                "Function breakpoint name, condition, and hit count round trip over YLDP");

    auto outputPayload = encodeOutputEvent("safe log output", DebugOutputCategory::Console);
    auto output = outputPayload ? decodeOutputEvent(bytesOf(*outputPayload))
                                : ProtocolResult<std::pair<Str, DebugOutputCategory>>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite, output && output->first == "safe log output" && output->second == DebugOutputCategory::Console,
                "Remote log point output retains its bounded text and category");

    RemoteSetVariableRequest setVariable{VariableReference{44}, "captured", "'updated'"};
    auto setVariablePayload = encodeSetVariableRequest(setVariable);
    auto decodedSetVariable = setVariablePayload
                                  ? decodeSetVariableRequest(bytesOf(*setVariablePayload))
                                  : ProtocolResult<RemoteSetVariableRequest>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decodedSetVariable && decodedSetVariable->reference == VariableReference{44} &&
                    decodedSetVariable->name == "captured" && decodedSetVariable->valueExpression == "'updated'",
                "Remote setVariable carries only an opaque scope handle, name, and bounded value expression");
}

void testRemoteRuntimePayloadMessages(TestSuite& suite) {
    auto errorPayload = encodeErrorMessage("bounded remote error");
    auto error = errorPayload ? decodeErrorMessage(bytesOf(*errorPayload))
                              : ProtocolResult<Str>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite, error && *error == "bounded remote error", "Remote error text round trips over YLDP");

    const std::array sourceBindings{BreakpointBinding{BreakpointId{3}, SourceId{5}, 17, 19, true, "bound", {}}};
    auto sourcePayload = encodeBreakpointBindings(sourceBindings);
    auto decodedSources = sourcePayload ? decodeBreakpointBindings(bytesOf(*sourcePayload))
                                        : ProtocolResult<Vec<BreakpointBinding>>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decodedSources && decodedSources->size() == 1 && decodedSources->front().id == BreakpointId{3} &&
                    decodedSources->front().requestedLine == 17 && decodedSources->front().line == 19 &&
                    decodedSources->front().verified,
                "Source breakpoint bindings retain resolved locations and verification state");

    const std::array functionBindings{
        BreakpointBinding{BreakpointId{7}, SourceId{9}, 0, 23, true, "resolved", Opt<Str>{"worker"}}};
    auto functionPayload = encodeFunctionBreakpointBindings(functionBindings);
    auto decodedFunctions = functionPayload ? decodeFunctionBreakpointBindings(bytesOf(*functionPayload))
                                            : ProtocolResult<Vec<BreakpointBinding>>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decodedFunctions && decodedFunctions->size() == 1 &&
                    decodedFunctions->front().functionName == Opt<Str>{"worker"} &&
                    decodedFunctions->front().line == 23,
                "Function breakpoint bindings retain their resolved symbol and line");

    const std::array threads{DebugThread{ThreadId{11}, "main", DebugThreadState::Paused}};
    auto threadPayload = encodeThreads(threads);
    auto decodedThreads = threadPayload ? decodeThreads(bytesOf(*threadPayload))
                                        : ProtocolResult<Vec<DebugThread>>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decodedThreads && decodedThreads->size() == 1 && decodedThreads->front().id == ThreadId{11} &&
                    decodedThreads->front().state == DebugThreadState::Paused,
                "Debug thread snapshots retain identity and lifecycle state");

    const DebugState state{StateId{13}, ThreadId{11}, "game", "Game VM", DebugThreadState::Running, true};
    auto statePayload = encodeDebugStateEvent(state);
    auto decodedState = statePayload ? decodeDebugStateEvent(bytesOf(*statePayload))
                                     : ProtocolResult<DebugState>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decodedState && decodedState->id == StateId{13} && decodedState->threadId == ThreadId{11} &&
                    decodedState->label == "Game VM" && decodedState->selected,
                "Debug state events retain their selected runtime identity");

    const std::array frames{RemoteStackFrame{
        DebugStackFrame{FrameId{17}, ThreadId{11}, "tick", SourceLocation{SourceId{5}, 29, 4, 8}, false},
        "/srv/game/main.lua", true}};
    auto framePayload = encodeStackFrames(frames);
    auto decodedFrames = framePayload ? decodeStackFrames(bytesOf(*framePayload))
                                      : ProtocolResult<Vec<RemoteStackFrame>>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decodedFrames && decodedFrames->size() == 1 && decodedFrames->front().frame.id == FrameId{17} &&
                    decodedFrames->front().frame.location.line == 29 && decodedFrames->front().sourceIsFile,
                "Remote stack frames retain source coordinates and source kind");

    const std::array scopes{DebugScope{"Locals", DebugScopeKind::Locals, VariableReference{21}, false}};
    auto scopePayload = encodeScopes(scopes);
    auto decodedScopes = scopePayload ? decodeScopes(bytesOf(*scopePayload))
                                      : ProtocolResult<Vec<DebugScope>>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decodedScopes && decodedScopes->size() == 1 &&
                    decodedScopes->front().variablesReference == VariableReference{21} &&
                    decodedScopes->front().kind == DebugScopeKind::Locals,
                "Remote scopes retain their opaque variable handle and category");

    const DebugVariable variable{"score", "42", "number", Opt<Str>{"score"}, VariableReference{31}, 2, 3};
    auto variablePayload = encodeVariable(variable);
    auto decodedVariable = variablePayload ? decodeVariable(bytesOf(*variablePayload))
                                           : ProtocolResult<DebugVariable>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decodedVariable && decodedVariable->name == "score" && decodedVariable->value == "42" &&
                    decodedVariable->evaluateName == Opt<Str>{"score"} && decodedVariable->namedVariables == 2 &&
                    decodedVariable->indexedVariables == 3,
                "Remote variables retain evaluation metadata and child counts");

    const std::array variables{variable};
    auto variablesPayload = encodeVariables(variables);
    auto decodedVariables = variablesPayload ? decodeVariables(bytesOf(*variablesPayload))
                                             : ProtocolResult<Vec<DebugVariable>>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite, decodedVariables && decodedVariables->size() == 1 && decodedVariables->front().type == "number",
                "Remote variable collections use the same bounded value codec");

    const DebugExceptionInfo exception{"runtime", "attempt to index nil", "always",
                                       DebugExceptionCategory::RuntimeError};
    auto exceptionPayload = encodeExceptionInfo(exception);
    auto decodedException = exceptionPayload ? decodeExceptionInfo(bytesOf(*exceptionPayload))
                                             : ProtocolResult<DebugExceptionInfo>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decodedException && decodedException->exceptionId == "runtime" &&
                    decodedException->description == "attempt to index nil" &&
                    decodedException->category == DebugExceptionCategory::RuntimeError,
                "Remote exception information retains its stable identity and category");

    const RemoteStoppedEvent stopped{DebugStopReason::Breakpoint, ThreadId{11}, PauseGeneration{37}};
    auto stoppedPayload = encodeStoppedEvent(stopped);
    auto decodedStopped = stoppedPayload ? decodeStoppedEvent(bytesOf(*stoppedPayload))
                                         : ProtocolResult<RemoteStoppedEvent>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decodedStopped && decodedStopped->reason == DebugStopReason::Breakpoint &&
                    decodedStopped->thread == ThreadId{11} && decodedStopped->generation == PauseGeneration{37},
                "Stopped events retain the pause generation used to reject stale handles");

    const RemoteTerminatedEvent terminated{DebugTerminationReason::RuntimeError,
                                           DebugError{DebugErrorCode::RuntimeFailure, "runtime failed", false}};
    auto terminatedPayload = encodeTerminatedEvent(terminated);
    auto decodedTerminated = terminatedPayload
                                 ? decodeTerminatedEvent(bytesOf(*terminatedPayload))
                                 : ProtocolResult<RemoteTerminatedEvent>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite,
                decodedTerminated && decodedTerminated->reason == DebugTerminationReason::RuntimeError &&
                    decodedTerminated->error && decodedTerminated->error->message == "runtime failed",
                "Terminated events retain their optional structured runtime error");
}

void testRemoteProtocolMutationCorpus(TestSuite& suite) {
    ProtocolFrame seed;
    seed.kind = ProtocolMessageKind::Response;
    seed.command = ProtocolCommand::Variables;
    seed.requestId = 9;
    seed.payload = {1, 2, 3, 4, 5};
    const auto encoded = encodeProtocolFrame(seed);
    usize handled = 0;
    if (encoded) {
        for (usize index = 0; index < encoded->size(); ++index) {
            Vec<u8> mutated = *encoded;
            mutated[index] ^= static_cast<u8>(0x5aU + static_cast<u8>(index));
            (void)decodeProtocolFrame(bytesOf(mutated));
            ++handled;
        }
        for (usize length = 0; length < encoded->size(); ++length) {
            ProtocolFrameDecoder decoder;
            (void)decoder.feed(std::span<const u8>(encoded->data(), length));
            (void)decoder.finish();
            ++handled;
        }
    }
    ASSERT_TRUE(suite, encoded && handled == encoded->size() * 2,
                "Deterministic mutation and truncation corpus completes without exceptions or unbounded allocation");
    ASSERT_EQ(suite, usize{64}, StrView(kProtocolSchemaSha256).size(),
              "Generated wire constants embed the exact schema SHA-256");
}

void testRemoteProtocolLoopbackTransport(TestSuite& suite) {
    auto listener = TcpListener::listen("127.0.0.1", 0);
    ASSERT_TRUE(suite, listener && listener->localPort() != 0,
                "Opt-in debug listener binds loopback and reports an ephemeral port");
    if (!listener) {
        return;
    }
    auto client = TcpConnection::connect("127.0.0.1", listener->localPort(), 3000);
    ASSERT_TRUE(suite, client.has_value(), "Debug client connects to the loopback listener within its timeout");
    if (!client) {
        return;
    }

    bool serverCompleted = false;
    std::thread server([&]() {
        auto accepted = listener->accept();
        if (!accepted) {
            return;
        }
        (void)accepted->setTimeouts(3000, 3000);
        auto received = accepted->receiveSome(64);
        if (!received || !accepted->sendAll(bytesOf(*received))) {
            return;
        }
        serverCompleted = true;
    });

    const Vec<u8> message{1, 3, 3, 7};
    const auto sent = client->sendAll(bytesOf(message));
    const auto echoed = client->receiveSome(64);
    client->close();
    listener->close();
    server.join();
    ASSERT_TRUE(suite, sent && echoed && *echoed == message && serverCompleted,
                "Loopback transport sends complete bounded byte sequences in both directions");
    ASSERT_TRUE(suite,
                isLoopbackAddress("127.0.0.1") && isLoopbackAddress("127.12.0.9") && !isLoopbackAddress("0.0.0.0") &&
                    !isLoopbackAddress("192.0.2.1"),
                "Loopback policy rejects wildcard and non-loopback bind addresses by default");
}

void testRuntimeDebugServerLoopback(TestSuite& suite) {
    DebugController controller;
    RuntimeDebugServer server(controller);

    DebugServerConfig config;
    auto disabled = server.start(config);
    ASSERT_TRUE(suite, !disabled && disabled.error().status == ProtocolStatus::InvalidState,
                "Remote server requires explicit enablement");

    config.enabled = true;
    config.authToken = "short";
    auto shortToken = server.start(config);
    ASSERT_TRUE(suite, !shortToken && shortToken.error().status == ProtocolStatus::InvalidArgument,
                "Remote server rejects short authentication tokens");

    config.authToken = "contract-token-1234";
    config.bindAddress = "0.0.0.0";
    auto wildcard = server.start(config);
    ASSERT_TRUE(suite, !wildcard && wildcard.error().status == ProtocolStatus::Unauthorized,
                "Remote server rejects non-loopback binding without authorization");

    config.bindAddress = "127.0.0.1";
    config.maxConnections = 2;
    auto multipleClients = server.start(config);
    ASSERT_TRUE(suite, !multipleClients && multipleClients.error().status == ProtocolStatus::InvalidArgument,
                "Remote server enforces its single-client resource contract");

    config.maxConnections = 1;
    config.maxFrameBytes = kProtocolHeaderSize - 1;
    auto invalidLimits = server.start(config);
    ASSERT_TRUE(suite, !invalidLimits && invalidLimits.error().status == ProtocolStatus::InvalidArgument,
                "Remote server rejects invalid frame limits");

    config.maxFrameBytes = kProtocolMaxFrameBytes;
    config.handshakeTimeoutMs = 3000;
    config.heartbeatIntervalMs = 250;
    config.idleTimeoutMs = 2000;
    auto started = server.start(config);
    ASSERT_TRUE(suite, started && server.running() && started->port != 0,
                "Remote server starts on an ephemeral loopback endpoint");
    ASSERT_TRUE(suite, !server.start(config), "Remote server rejects a second start while running");
    if (!started) {
        return;
    }

    {
        auto malformed = TcpConnection::connect(started->address, started->port, 3000);
        ASSERT_TRUE(suite, malformed.has_value(), "Malformed handshake client connects to the server");
        if (malformed) {
            (void)malformed->setTimeouts(3000, 3000);
            ProtocolFrame request;
            request.kind = ProtocolMessageKind::Request;
            request.command = ProtocolCommand::Threads;
            request.requestId = 1;
            ASSERT_TRUE(suite, sendFrame(*malformed, std::move(request)).has_value(),
                        "Malformed handshake frame reaches the server");
            malformed->close();
        }
    }
    ASSERT_TRUE(suite, waitForServerStats(server, 1, 0, 0, 1),
                "Malformed handshake increments the protocol failure counter");

    {
        auto malformedHello = TcpConnection::connect(started->address, started->port, 3000);
        ASSERT_TRUE(suite, malformedHello.has_value(), "Malformed hello client connects to the server");
        if (malformedHello) {
            (void)malformedHello->setTimeouts(3000, 3000);
            ProtocolFrame frame;
            frame.kind = ProtocolMessageKind::Hello;
            frame.requestId = 1;
            frame.payload = {0xffU};
            ProtocolFrameReader malformedReader;
            auto sent = sendFrame(*malformedHello, std::move(frame));
            auto rejected = sent ? malformedReader.receive(*malformedHello)
                                 : ProtocolResult<ProtocolFrame>(std::unexpected(sent.error()));
            ASSERT_TRUE(suite, rejected && rejected->status == ProtocolStatus::ProtocolError,
                        "Remote server rejects a malformed hello payload with a protocol response");
            malformedHello->close();
        }
    }
    ASSERT_TRUE(suite, waitForServerStats(server, 2, 0, 0, 2),
                "Malformed hello payload increments the protocol failure counter");

    {
        auto incompatible = TcpConnection::connect(started->address, started->port, 3000);
        ASSERT_TRUE(suite, incompatible.has_value(), "Version-mismatch client connects to the server");
        if (incompatible) {
            (void)incompatible->setTimeouts(3000, 3000);
            ProtocolFrameReader incompatibleReader;
            auto rejected = exchangeHello(*incompatible, incompatibleReader, config.authToken, 1, 2, 2);
            ASSERT_TRUE(suite, rejected && rejected->status == ProtocolStatus::VersionMismatch,
                        "Remote server rejects an incompatible protocol major version");
            incompatible->close();
        }
    }

    {
        auto missingState = TcpConnection::connect(started->address, started->port, 3000);
        ASSERT_TRUE(suite, missingState.has_value(), "Missing-state client connects to the server");
        if (missingState) {
            (void)missingState->setTimeouts(3000, 3000);
            ProtocolFrameReader missingStateReader;
            auto rejected = exchangeHello(*missingState, missingStateReader, config.authToken, 1, kProtocolVersionMajor,
                                          kProtocolVersionMajor, "missing-runtime-state");
            ASSERT_TRUE(suite, rejected && rejected->status == ProtocolStatus::InvalidArgument,
                        "Remote server rejects a requested runtime state that does not exist");
            missingState->close();
        }
    }

    {
        auto unauthorized = TcpConnection::connect(started->address, started->port, 3000);
        ASSERT_TRUE(suite, unauthorized.has_value(), "Unauthorized client connects to the loopback server");
        if (unauthorized) {
            (void)unauthorized->setTimeouts(3000, 3000);
            ProtocolFrameReader reader;
            auto rejected = exchangeHello(*unauthorized, reader, "wrong-token-12345");
            ASSERT_TRUE(suite,
                        rejected && rejected->kind == ProtocolMessageKind::HelloAck &&
                            rejected->status == ProtocolStatus::Unauthorized,
                        "Remote server rejects an incorrect token with a bounded acknowledgement");
            unauthorized->close();
        }
    }
    ASSERT_TRUE(suite, waitForServerStats(server, 5, 0, 1, 2),
                "Authentication failure is recorded without stopping the listener");

    auto client = TcpConnection::connect(started->address, started->port, 3000);
    ASSERT_TRUE(suite, client.has_value(), "Authenticated client connects after rejected sessions");
    if (!client) {
        return;
    }
    ASSERT_TRUE(suite, client->setTimeouts(3000, 3000).has_value(), "Authenticated client configures bounded I/O");
    ProtocolFrameReader reader;
    auto acknowledged = exchangeHello(*client, reader, config.authToken);
    ASSERT_TRUE(suite,
                acknowledged && acknowledged->kind == ProtocolMessageKind::HelloAck &&
                    acknowledged->status == ProtocolStatus::Okay,
                "Authenticated handshake negotiates a debugger session");

    auto serverPing = reader.receive(*client);
    ASSERT_TRUE(suite, serverPing && serverPing->kind == ProtocolMessageKind::Ping,
                "Idle authenticated session receives a bounded server heartbeat");
    if (serverPing) {
        ProtocolFrame heartbeat;
        heartbeat.kind = ProtocolMessageKind::Pong;
        heartbeat.requestId = serverPing->requestId;
        ASSERT_TRUE(suite, sendFrame(*client, std::move(heartbeat)).has_value(),
                    "Authenticated client acknowledges the server heartbeat");
    }

    ProtocolFrame ping;
    ping.kind = ProtocolMessageKind::Ping;
    ping.requestId = 7;
    auto pingSent = sendFrame(*client, std::move(ping));
    auto pong = pingSent ? reader.receive(*client) : ProtocolResult<ProtocolFrame>(std::unexpected(pingSent.error()));
    ASSERT_TRUE(suite, pong && pong->kind == ProtocolMessageKind::Pong && pong->requestId == 7,
                "Authenticated session answers heartbeat pings");

    u64 requestId = 10;
    auto expectResponse = [&](ProtocolCommand command, Vec<u8> payload, ProtocolStatus expected) {
        auto response = exchangeRequest(*client, reader, command, requestId++, std::move(payload));
        ASSERT_TRUE(suite,
                    response && response->kind == ProtocolMessageKind::Response && response->command == command &&
                        response->status == expected,
                    "Remote command returns the expected bounded response");
    };

    RemoteBreakpointRequest breakpoints{"/tmp/remote-contract.lua", {}};
    expectResponse(ProtocolCommand::SetBreakpoints, encodeBreakpointRequest(breakpoints).value_or(Vec<u8>{}),
                   ProtocolStatus::Okay);
    expectResponse(ProtocolCommand::SetAdvancedBreakpoints,
                   encodeAdvancedBreakpointRequest(breakpoints).value_or(Vec<u8>{}), ProtocolStatus::Okay);
    expectResponse(ProtocolCommand::SetFunctionBreakpoints, encodeFunctionBreakpointRequest({}).value_or(Vec<u8>{}),
                   ProtocolStatus::Okay);

    EngineContext context;
    UPtr<LuaState> state = LuaState::create(context);
    const StateId stateId = controller.registerState(*state, "remote-contract", "Remote Contract");
    auto stateStarted = reader.receive(*client);
    auto threadStarted = reader.receive(*client);
    ASSERT_TRUE(suite,
                stateId.valid() && stateStarted && threadStarted && stateStarted->kind == ProtocolMessageKind::Event &&
                    threadStarted->kind == ProtocolMessageKind::Event,
                "Runtime state registration publishes state and thread events");

    expectResponse(ProtocolCommand::Threads, {}, ProtocolStatus::Okay);
    expectResponse(ProtocolCommand::States, {}, ProtocolStatus::Okay);
    expectResponse(ProtocolCommand::SelectState, encodeStateRequest(stateId).value_or(Vec<u8>{}), ProtocolStatus::Okay);
    expectResponse(ProtocolCommand::SelectState, encodeStateRequest(StateId{999}).value_or(Vec<u8>{}),
                   ProtocolStatus::NotFound);
    expectResponse(ProtocolCommand::StackTrace,
                   encodeStackTraceRequest({DebugController::mainThreadId(), 0, 8}).value_or(Vec<u8>{}),
                   ProtocolStatus::InvalidState);
    expectResponse(ProtocolCommand::Scopes, encodeFrameRequest(FrameId{999}).value_or(Vec<u8>{}),
                   ProtocolStatus::InvalidState);
    expectResponse(ProtocolCommand::Variables,
                   encodeVariablesRequest({VariableReference{999}, 0, 8, DebugVariableFilter::All}).value_or(Vec<u8>{}),
                   ProtocolStatus::InvalidState);
    expectResponse(ProtocolCommand::Evaluate, encodeEvaluateRequest({FrameId{999}, "1 + 1"}).value_or(Vec<u8>{}),
                   ProtocolStatus::InvalidState);
    expectResponse(ProtocolCommand::EvaluateSideEffects, {}, ProtocolStatus::Unauthorized);
    expectResponse(ProtocolCommand::SetVariable, {}, ProtocolStatus::Unauthorized);
    expectResponse(ProtocolCommand::ExceptionInfo,
                   encodeThreadRequest(DebugController::mainThreadId()).value_or(Vec<u8>{}),
                   ProtocolStatus::InvalidState);
    expectResponse(ProtocolCommand::SetExceptionBreakpoints, encodeBooleanRequest(true).value_or(Vec<u8>{}),
                   ProtocolStatus::Okay);
    expectResponse(static_cast<ProtocolCommand>(0xffffU), {}, ProtocolStatus::NotSupported);

    expectResponse(ProtocolCommand::Pause, encodeThreadRequest(DebugController::mainThreadId()).value_or(Vec<u8>{}),
                   ProtocolStatus::Okay);
    ASSERT_TRUE(suite, controller.notifySuspended(DebugStopReason::Pause).has_value(),
                "Owner safepoint confirms the remote pause request");
    auto stopped = reader.receive(*client);
    ASSERT_TRUE(suite,
                stopped && stopped->kind == ProtocolMessageKind::Event &&
                    stopped->command == static_cast<ProtocolCommand>(ProtocolEvent::Stopped),
                "Remote server publishes the stopped lifecycle event");

    const std::array stepCommands{ProtocolCommand::Next, ProtocolCommand::StepIn, ProtocolCommand::StepOut};
    for (ProtocolCommand command : stepCommands) {
        expectResponse(command, encodeThreadRequest(DebugController::mainThreadId()).value_or(Vec<u8>{}),
                       ProtocolStatus::Okay);
        ASSERT_TRUE(suite, controller.confirmResumed().has_value(), "Owner confirms the remote step resume");
        auto continued = reader.receive(*client);
        ASSERT_TRUE(suite,
                    continued && continued->kind == ProtocolMessageKind::Event &&
                        continued->command == static_cast<ProtocolCommand>(ProtocolEvent::Continued),
                    "Remote server publishes the continued lifecycle event");
        ASSERT_TRUE(suite, controller.notifySuspended(DebugStopReason::Step).has_value(),
                    "Owner reports the next remote step suspension");
        stopped = reader.receive(*client);
        ASSERT_TRUE(suite, stopped && stopped->command == static_cast<ProtocolCommand>(ProtocolEvent::Stopped),
                    "Remote server publishes each step completion event");
    }

    expectResponse(ProtocolCommand::Continue, encodeThreadRequest(DebugController::mainThreadId()).value_or(Vec<u8>{}),
                   ProtocolStatus::Okay);
    ASSERT_TRUE(suite, controller.confirmResumed().has_value(), "Owner confirms the final remote continue");
    auto continued = reader.receive(*client);
    ASSERT_TRUE(suite, continued && continued->command == static_cast<ProtocolCommand>(ProtocolEvent::Continued),
                "Remote server publishes the final continued event");

    controller.unregisterState(*state);
    auto stateExited = reader.receive(*client);
    auto threadExited = reader.receive(*client);
    ASSERT_TRUE(suite,
                stateExited && threadExited && stateExited->kind == ProtocolMessageKind::Event &&
                    threadExited->kind == ProtocolMessageKind::Event,
                "Runtime state removal publishes state and thread exit events");

    const u64 duplicateId = requestId++;
    auto first = exchangeRequest(*client, reader, ProtocolCommand::Threads, duplicateId);
    auto duplicate = exchangeRequest(*client, reader, ProtocolCommand::Threads, duplicateId);
    ASSERT_TRUE(suite,
                first && first->status == ProtocolStatus::Okay && duplicate &&
                    duplicate->status == ProtocolStatus::ProtocolError,
                "Remote server rejects duplicate request identifiers without dropping the session");

    controller.notifyProgramTerminated(DebugTerminationReason::Completed);
    auto terminated = reader.receive(*client);
    ASSERT_TRUE(suite,
                terminated && terminated->kind == ProtocolMessageKind::Event &&
                    terminated->command == static_cast<ProtocolCommand>(ProtocolEvent::Terminated),
                "Remote server publishes the terminated lifecycle event");

    auto detached = exchangeRequest(*client, reader, ProtocolCommand::Detach, requestId,
                                    encodeBooleanRequest(false).value_or(Vec<u8>{}));
    ASSERT_TRUE(suite, detached && detached->status == ProtocolStatus::Okay,
                "Remote detach receives an acknowledgement before disconnecting");
    client->close();
    ASSERT_TRUE(suite, waitForServerStats(server, 6, 1, 1, 2),
                "Remote server records accepted, authenticated, rejected, and malformed sessions");

    server.stop();
    ASSERT_TRUE(suite, !server.running() && server.endpoint().port == 0,
                "Stopping the remote server clears its published endpoint");

    DebugController busyController;
    auto heldSession = busyController.attachSession();
    RuntimeDebugServer busyServer(busyController);
    DebugServerConfig busyConfig = config;
    auto busyStarted = busyServer.start(busyConfig);
    ASSERT_TRUE(suite, heldSession && busyStarted, "Busy-session fixture starts with a held debugger attachment");
    if (busyStarted) {
        auto busyClient = TcpConnection::connect(busyStarted->address, busyStarted->port, 3000);
        ASSERT_TRUE(suite, busyClient.has_value(), "Busy-session client connects to the remote server");
        if (busyClient) {
            (void)busyClient->setTimeouts(3000, 3000);
            ProtocolFrameReader busyReader;
            auto rejected = exchangeHello(*busyClient, busyReader, busyConfig.authToken);
            ASSERT_TRUE(suite, rejected && rejected->status == ProtocolStatus::Busy,
                        "Remote server rejects a second debugger attachment as busy");
            busyClient->close();
        }
    }
    busyServer.stop();

    ASSERT_TRUE(suite, busyController.configurationDone().has_value(),
                "Held debugger session advances beyond the write-policy configuration phase");
    busyStarted = busyServer.start(busyConfig);
    ASSERT_TRUE(suite, busyStarted.has_value(), "Write-policy rejection fixture restarts its listener");
    if (busyStarted) {
        auto policyClient = TcpConnection::connect(busyStarted->address, busyStarted->port, 3000);
        ASSERT_TRUE(suite, policyClient.has_value(), "Write-policy client connects to the remote server");
        if (policyClient) {
            (void)policyClient->setTimeouts(3000, 3000);
            ProtocolFrameReader policyReader;
            auto rejected = exchangeHello(*policyClient, policyReader, busyConfig.authToken);
            ASSERT_TRUE(suite, rejected && rejected->status == ProtocolStatus::InvalidState,
                        "Remote server rejects policy changes after configurationDone");
            policyClient->close();
        }
    }
    busyServer.stop();
}

void registerDebuggerRemoteProtocolTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "Bounded Primitives", testRemoteProtocolPrimitives);
    registry.registerTest(kSuiteName, "Incremental Frames", testRemoteProtocolFrames);
    registry.registerTest(kSuiteName, "Malformed Inputs", testRemoteProtocolMalformedInputs);
    registry.registerTest(kSuiteName, "Versioned Handshake", testRemoteProtocolHandshake);
    registry.registerTest(kSuiteName, "Advanced Breakpoint Messages", testRemoteAdvancedBreakpointMessages);
    registry.registerTest(kSuiteName, "Runtime Payload Messages", testRemoteRuntimePayloadMessages);
    registry.registerTest(kSuiteName, "Mutation Corpus", testRemoteProtocolMutationCorpus);
    registry.registerTest(kSuiteName, "Loopback TCP Transport", testRemoteProtocolLoopbackTransport);
    registry.registerTest(kSuiteName, "Authenticated Runtime Server", testRuntimeDebugServerLoopback);
}
