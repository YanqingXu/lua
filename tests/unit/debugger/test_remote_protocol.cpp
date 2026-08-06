/**
 * @file test_remote_protocol.cpp
 * @brief YLDP framing, compatibility, limits, and malformed-input tests.
 */

#include "../framework/test_framework.hpp"

#include "debugger/remote_protocol.hpp"
#include "debugger/remote_transport.hpp"

#include <array>
#include <span>
#include <thread>

using namespace Lua;
using namespace Lua::Debugger::Remote;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Debugger Remote Protocol";

std::span<const u8> bytesOf(const Vec<u8>& bytes) {
    return std::span<const u8>(bytes.data(), bytes.size());
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
    ASSERT_TRUE(suite, value8 && *value8 == 0x7fU && value16 && *value16 == 0x1234U && value32 &&
                           *value32 == 0x89abcdefU && value64 && *value64 == 0x0123456789abcdefULL,
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
    ASSERT_TRUE(suite, decoded && decoded->major == kProtocolVersionMajor &&
                           decoded->kind == ProtocolMessageKind::Request && decoded->command == ProtocolCommand::Evaluate &&
                           decoded->requestId == original.requestId && decoded->payload == original.payload,
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
    ASSERT_TRUE(suite, fragments.size() == 1 && fragments[0].requestId == original.requestId &&
                           fragmented.finish().has_value(),
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
    const auto decoded = payload ? decodeHello(bytesOf(*payload))
                                 : ProtocolResult<ProtocolHello>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite, decoded && decoded->clientName == hello.clientName && decoded->authToken == hello.authToken &&
                           decoded->stateSelector == "game",
                "YLDP hello round trips version, authentication, capability, and state selection fields");

    const u64 available = capabilityBit(ProtocolCapability::Breakpoints) |
                          capabilityBit(ProtocolCapability::GlobalPauseOnly);
    const auto negotiated = decoded ? negotiateHello(*decoded, available, 73, "1.2.3")
                                    : ProtocolResult<ProtocolHelloAck>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite, negotiated && negotiated->sessionId == 73 &&
                           negotiated->capabilities == capabilityBit(ProtocolCapability::Breakpoints),
                "Handshake selects only the intersection of requested and available capabilities");
    const auto ackPayload = negotiated ? encodeHelloAck(*negotiated)
                                       : ProtocolResult<Vec<u8>>(std::unexpected(ProtocolError{}));
    const auto ack = ackPayload ? decodeHelloAck(bytesOf(*ackPayload))
                                : ProtocolResult<ProtocolHelloAck>(std::unexpected(ProtocolError{}));
    ASSERT_TRUE(suite, ack && ack->serverVersion == "1.2.3" && ack->serverName == "YanLua Runtime",
                "Handshake acknowledgement carries stable server metadata");
    ASSERT_TRUE(suite, !ackPayload || Str(reinterpret_cast<const char*>(ackPayload->data()), ackPayload->size())
                                              .find(hello.authToken) == Str::npos,
                "Authentication token is never echoed by the server handshake");

    ProtocolHello incompatible = hello;
    incompatible.minimumMajor = 2;
    incompatible.maximumMajor = 2;
    const auto mismatch = negotiateHello(incompatible, available, 1, "1.0.0");
    ASSERT_TRUE(suite, !mismatch && mismatch.error().status == ProtocolStatus::VersionMismatch,
                "Incompatible protocol major versions fail with a stable status");
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
    ASSERT_TRUE(suite, isLoopbackAddress("127.0.0.1") && isLoopbackAddress("127.12.0.9") &&
                           !isLoopbackAddress("0.0.0.0") && !isLoopbackAddress("192.0.2.1"),
                "Loopback policy rejects wildcard and non-loopback bind addresses by default");
}

void registerDebuggerRemoteProtocolTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "Bounded Primitives", testRemoteProtocolPrimitives);
    registry.registerTest(kSuiteName, "Incremental Frames", testRemoteProtocolFrames);
    registry.registerTest(kSuiteName, "Malformed Inputs", testRemoteProtocolMalformedInputs);
    registry.registerTest(kSuiteName, "Versioned Handshake", testRemoteProtocolHandshake);
    registry.registerTest(kSuiteName, "Mutation Corpus", testRemoteProtocolMutationCorpus);
    registry.registerTest(kSuiteName, "Loopback TCP Transport", testRemoteProtocolLoopbackTransport);
}
