#pragma once

/**
 * @file remote_protocol.hpp
 * @brief Versioned, bounded wire primitives for the YanLua Debug Protocol.
 */

#include "debugger/remote_protocol_generated.hpp"

#include <expected>
#include <span>

namespace Lua::Debugger::Remote {

struct ProtocolError {
    ProtocolStatus status = ProtocolStatus::ProtocolError;
    Str message;
    usize offset = 0;
};

template <typename T> using ProtocolResult = std::expected<T, ProtocolError>;

struct ProtocolFrame {
    u16 major = kProtocolVersionMajor;
    u16 minor = kProtocolVersionMinor;
    ProtocolMessageKind kind = ProtocolMessageKind::Request;
    u8 flags = 0;
    ProtocolCommand command = ProtocolCommand::None;
    u64 requestId = 0;
    ProtocolStatus status = ProtocolStatus::Okay;
    Vec<u8> payload;
};

class ProtocolWriter {
public:
    explicit ProtocolWriter(usize maxBytes = kProtocolMaxFrameBytes);

    void writeU8(u8 value);
    void writeU16(u16 value);
    void writeU32(u32 value);
    void writeU64(u64 value);
    void writeI64(i64 value);
    void writeBool(bool value);
    void writeString(StrView value);
    void writeBytes(std::span<const u8> value);

    [[nodiscard]] ProtocolResult<Vec<u8>> finish() &&;

private:
    bool reserve(usize bytes);

    usize maxBytes_;
    Vec<u8> bytes_;
    Opt<ProtocolError> error_;
};

class ProtocolReader {
public:
    explicit ProtocolReader(std::span<const u8> bytes, usize maxStringBytes = kProtocolMaxStringBytes,
                            usize maxCollectionItems = kProtocolMaxCollectionItems);

    [[nodiscard]] ProtocolResult<u8> readU8();
    [[nodiscard]] ProtocolResult<u16> readU16();
    [[nodiscard]] ProtocolResult<u32> readU32();
    [[nodiscard]] ProtocolResult<u64> readU64();
    [[nodiscard]] ProtocolResult<i64> readI64();
    [[nodiscard]] ProtocolResult<bool> readBool();
    [[nodiscard]] ProtocolResult<Str> readString();
    [[nodiscard]] ProtocolResult<Vec<u8>> readBytes();
    [[nodiscard]] ProtocolResult<usize> readCount();
    [[nodiscard]] ProtocolResult<void> finish() const;
    [[nodiscard]] usize remaining() const noexcept;

private:
    [[nodiscard]] ProtocolError failure(Str message) const;
    [[nodiscard]] bool available(usize bytes) const noexcept;

    std::span<const u8> bytes_;
    usize offset_ = 0;
    usize maxStringBytes_;
    usize maxCollectionItems_;
};

[[nodiscard]] ProtocolResult<Vec<u8>> encodeProtocolFrame(const ProtocolFrame& frame,
                                                          usize maxFrameBytes = kProtocolMaxFrameBytes);
[[nodiscard]] ProtocolResult<ProtocolFrame> decodeProtocolFrame(std::span<const u8> frameBytes,
                                                                usize maxFrameBytes = kProtocolMaxFrameBytes);

class ProtocolFrameDecoder {
public:
    explicit ProtocolFrameDecoder(usize maxFrameBytes = kProtocolMaxFrameBytes);

    [[nodiscard]] ProtocolResult<Vec<ProtocolFrame>> feed(std::span<const u8> bytes);
    [[nodiscard]] ProtocolResult<Vec<ProtocolFrame>> feed(StrView bytes);
    [[nodiscard]] ProtocolResult<void> finish() const;
    void reset() noexcept;

private:
    usize maxFrameBytes_;
    Vec<u8> buffer_;
    Opt<ProtocolError> error_;
};

struct ProtocolHello {
    u16 minimumMajor = kProtocolVersionMajor;
    u16 maximumMajor = kProtocolVersionMajor;
    u16 minimumMinor = 0;
    u16 maximumMinor = kProtocolVersionMinor;
    Str clientName;
    Str clientVersion;
    Str authToken;
    u64 requestedCapabilities = 0;
    Str stateSelector;
};

struct ProtocolHelloAck {
    u16 selectedMajor = kProtocolVersionMajor;
    u16 selectedMinor = kProtocolVersionMinor;
    u64 capabilities = 0;
    u64 sessionId = 0;
    u32 heartbeatIntervalMs = kProtocolHeartbeatIntervalMs;
    Str serverName = "YanLua Runtime";
    Str serverVersion;
};

[[nodiscard]] ProtocolResult<Vec<u8>> encodeHello(const ProtocolHello& hello);
[[nodiscard]] ProtocolResult<ProtocolHello> decodeHello(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<Vec<u8>> encodeHelloAck(const ProtocolHelloAck& hello);
[[nodiscard]] ProtocolResult<ProtocolHelloAck> decodeHelloAck(std::span<const u8> payload);
[[nodiscard]] ProtocolResult<ProtocolHelloAck> negotiateHello(const ProtocolHello& hello, u64 availableCapabilities,
                                                              u64 sessionId, StrView serverVersion);

} // namespace Lua::Debugger::Remote
