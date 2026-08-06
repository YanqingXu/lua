/**
 * @file remote_protocol.cpp
 * @brief Bounded network-byte-order framing and handshake for YLDP.
 */

#include "debugger/remote_protocol.hpp"

#include <algorithm>
#include <cstring>
#include <limits>

namespace Lua::Debugger::Remote {

namespace {

constexpr usize kFrameLengthPrefixSize = 4;
constexpr usize kFrameBytesAfterLength = kProtocolHeaderSize - kFrameLengthPrefixSize;

ProtocolError resourceError(Str message, usize offset = 0) {
    return {ProtocolStatus::ResourceLimit, std::move(message), offset};
}

ProtocolError protocolError(Str message, usize offset = 0) {
    return {ProtocolStatus::ProtocolError, std::move(message), offset};
}

u16 loadU16(const u8* bytes) noexcept {
    return static_cast<u16>((static_cast<u16>(bytes[0]) << 8U) | static_cast<u16>(bytes[1]));
}

u32 loadU32(const u8* bytes) noexcept {
    return (static_cast<u32>(bytes[0]) << 24U) | (static_cast<u32>(bytes[1]) << 16U) |
           (static_cast<u32>(bytes[2]) << 8U) | static_cast<u32>(bytes[3]);
}

u64 loadU64(const u8* bytes) noexcept {
    u64 result = 0;
    for (usize index = 0; index < 8; ++index) {
        result = (result << 8U) | static_cast<u64>(bytes[index]);
    }
    return result;
}

bool knownKind(ProtocolMessageKind kind) noexcept {
    return kind >= ProtocolMessageKind::Hello && kind <= ProtocolMessageKind::Goodbye;
}

} // namespace

ProtocolWriter::ProtocolWriter(usize maxBytes) : maxBytes_(maxBytes) {
    bytes_.reserve(std::min(maxBytes, usize{256}));
}

bool ProtocolWriter::reserve(usize bytes) {
    if (error_) {
        return false;
    }
    if (bytes > maxBytes_ || bytes_.size() > maxBytes_ - bytes) {
        error_ = resourceError("protocol payload exceeds configured byte limit", bytes_.size());
        return false;
    }
    return true;
}

void ProtocolWriter::writeU8(u8 value) {
    if (reserve(1)) {
        bytes_.push_back(value);
    }
}

void ProtocolWriter::writeU16(u16 value) {
    if (!reserve(2)) {
        return;
    }
    bytes_.push_back(static_cast<u8>((value >> 8U) & 0xffU));
    bytes_.push_back(static_cast<u8>(value & 0xffU));
}

void ProtocolWriter::writeU32(u32 value) {
    if (!reserve(4)) {
        return;
    }
    for (i32 shift = 24; shift >= 0; shift -= 8) {
        bytes_.push_back(static_cast<u8>((value >> static_cast<u32>(shift)) & 0xffU));
    }
}

void ProtocolWriter::writeU64(u64 value) {
    if (!reserve(8)) {
        return;
    }
    for (i32 shift = 56; shift >= 0; shift -= 8) {
        bytes_.push_back(static_cast<u8>((value >> static_cast<u32>(shift)) & 0xffU));
    }
}

void ProtocolWriter::writeI64(i64 value) {
    writeU64(static_cast<u64>(value));
}

void ProtocolWriter::writeBool(bool value) {
    writeU8(value ? 1U : 0U);
}

void ProtocolWriter::writeString(StrView value) {
    if (value.size() > kProtocolMaxStringBytes || value.size() > std::numeric_limits<u32>::max()) {
        if (!error_) {
            error_ = resourceError("protocol string exceeds configured byte limit", bytes_.size());
        }
        return;
    }
    writeU32(static_cast<u32>(value.size()));
    if (!reserve(value.size())) {
        return;
    }
    if (!value.empty()) {
        bytes_.insert(bytes_.end(), reinterpret_cast<const u8*>(value.data()),
                      reinterpret_cast<const u8*>(value.data()) + value.size());
    }
}

void ProtocolWriter::writeBytes(std::span<const u8> value) {
    if (value.size() > std::numeric_limits<u32>::max()) {
        if (!error_) {
            error_ = resourceError("protocol byte vector exceeds configured limit", bytes_.size());
        }
        return;
    }
    writeU32(static_cast<u32>(value.size()));
    if (reserve(value.size())) {
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }
}

ProtocolResult<Vec<u8>> ProtocolWriter::finish() && {
    if (error_) {
        return std::unexpected(std::move(*error_));
    }
    return std::move(bytes_);
}

ProtocolReader::ProtocolReader(std::span<const u8> bytes, usize maxStringBytes, usize maxCollectionItems)
    : bytes_(bytes), maxStringBytes_(maxStringBytes), maxCollectionItems_(maxCollectionItems) {}

ProtocolError ProtocolReader::failure(Str message) const {
    return protocolError(std::move(message), offset_);
}

bool ProtocolReader::available(usize bytes) const noexcept {
    return bytes <= bytes_.size() - std::min(offset_, bytes_.size());
}

ProtocolResult<u8> ProtocolReader::readU8() {
    if (!available(1)) {
        return std::unexpected(failure("unexpected end of protocol payload"));
    }
    return bytes_[offset_++];
}

ProtocolResult<u16> ProtocolReader::readU16() {
    if (!available(2)) {
        return std::unexpected(failure("unexpected end of protocol payload"));
    }
    const u16 value = loadU16(bytes_.data() + offset_);
    offset_ += 2;
    return value;
}

ProtocolResult<u32> ProtocolReader::readU32() {
    if (!available(4)) {
        return std::unexpected(failure("unexpected end of protocol payload"));
    }
    const u32 value = loadU32(bytes_.data() + offset_);
    offset_ += 4;
    return value;
}

ProtocolResult<u64> ProtocolReader::readU64() {
    if (!available(8)) {
        return std::unexpected(failure("unexpected end of protocol payload"));
    }
    const u64 value = loadU64(bytes_.data() + offset_);
    offset_ += 8;
    return value;
}

ProtocolResult<i64> ProtocolReader::readI64() {
    auto value = readU64();
    if (!value) {
        return std::unexpected(value.error());
    }
    return static_cast<i64>(*value);
}

ProtocolResult<bool> ProtocolReader::readBool() {
    auto value = readU8();
    if (!value) {
        return std::unexpected(value.error());
    }
    if (*value > 1U) {
        return std::unexpected(failure("protocol boolean must be zero or one"));
    }
    return *value != 0;
}

ProtocolResult<Str> ProtocolReader::readString() {
    auto length = readU32();
    if (!length) {
        return std::unexpected(length.error());
    }
    if (*length > maxStringBytes_) {
        return std::unexpected(resourceError("protocol string exceeds configured byte limit", offset_));
    }
    if (!available(*length)) {
        return std::unexpected(failure("protocol string is truncated"));
    }
    Str value(reinterpret_cast<const char*>(bytes_.data() + offset_), *length);
    offset_ += *length;
    return value;
}

ProtocolResult<Vec<u8>> ProtocolReader::readBytes() {
    auto length = readU32();
    if (!length) {
        return std::unexpected(length.error());
    }
    if (*length > kProtocolMaxFrameBytes) {
        return std::unexpected(resourceError("protocol byte vector exceeds configured limit", offset_));
    }
    if (!available(*length)) {
        return std::unexpected(failure("protocol byte vector is truncated"));
    }
    Vec<u8> value(bytes_.begin() + static_cast<isize>(offset_),
                  bytes_.begin() + static_cast<isize>(offset_ + *length));
    offset_ += *length;
    return value;
}

ProtocolResult<usize> ProtocolReader::readCount() {
    auto count = readU32();
    if (!count) {
        return std::unexpected(count.error());
    }
    if (*count > maxCollectionItems_) {
        return std::unexpected(resourceError("protocol collection exceeds configured item limit", offset_));
    }
    return static_cast<usize>(*count);
}

ProtocolResult<void> ProtocolReader::finish() const {
    if (offset_ != bytes_.size()) {
        return std::unexpected(protocolError("protocol payload contains trailing bytes", offset_));
    }
    return {};
}

usize ProtocolReader::remaining() const noexcept {
    return bytes_.size() - std::min(offset_, bytes_.size());
}

ProtocolResult<Vec<u8>> encodeProtocolFrame(const ProtocolFrame& frame, usize maxFrameBytes) {
    if (!knownKind(frame.kind)) {
        return std::unexpected(protocolError("unknown protocol message kind"));
    }
    if (maxFrameBytes < kProtocolHeaderSize || frame.payload.size() > maxFrameBytes - kProtocolHeaderSize) {
        return std::unexpected(resourceError("protocol frame exceeds configured byte limit"));
    }
    const usize totalBytes = kProtocolHeaderSize + frame.payload.size();
    if (totalBytes - kFrameLengthPrefixSize > std::numeric_limits<u32>::max()) {
        return std::unexpected(resourceError("protocol frame cannot be represented by the length prefix"));
    }
    ProtocolWriter writer(totalBytes);
    writer.writeU32(static_cast<u32>(totalBytes - kFrameLengthPrefixSize));
    writer.writeU32(kProtocolMagic);
    writer.writeU16(frame.major);
    writer.writeU16(frame.minor);
    writer.writeU8(static_cast<u8>(frame.kind));
    writer.writeU8(frame.flags);
    writer.writeU16(static_cast<u16>(frame.command));
    writer.writeU64(frame.requestId);
    writer.writeU16(static_cast<u16>(frame.status));
    writer.writeU16(0);
    for (u8 byte : frame.payload) {
        writer.writeU8(byte);
    }
    return std::move(writer).finish();
}

ProtocolResult<ProtocolFrame> decodeProtocolFrame(std::span<const u8> frameBytes, usize maxFrameBytes) {
    if (frameBytes.size() < kProtocolHeaderSize) {
        return std::unexpected(protocolError("protocol frame is shorter than its fixed header"));
    }
    if (frameBytes.size() > maxFrameBytes) {
        return std::unexpected(resourceError("protocol frame exceeds configured byte limit"));
    }
    const u32 declared = loadU32(frameBytes.data());
    if (declared != frameBytes.size() - kFrameLengthPrefixSize) {
        return std::unexpected(protocolError("protocol frame length does not match its prefix"));
    }
    if (loadU32(frameBytes.data() + 4) != kProtocolMagic) {
        return std::unexpected(protocolError("protocol frame magic is invalid", 4));
    }
    ProtocolFrame result;
    result.major = loadU16(frameBytes.data() + 8);
    result.minor = loadU16(frameBytes.data() + 10);
    result.kind = static_cast<ProtocolMessageKind>(frameBytes[12]);
    result.flags = frameBytes[13];
    result.command = static_cast<ProtocolCommand>(loadU16(frameBytes.data() + 14));
    result.requestId = loadU64(frameBytes.data() + 16);
    result.status = static_cast<ProtocolStatus>(loadU16(frameBytes.data() + 24));
    if (loadU16(frameBytes.data() + 26) != 0) {
        return std::unexpected(protocolError("protocol reserved header bits must be zero", 26));
    }
    if (!knownKind(result.kind)) {
        return std::unexpected(protocolError("unknown protocol message kind", 12));
    }
    result.payload.assign(frameBytes.begin() + static_cast<isize>(kProtocolHeaderSize), frameBytes.end());
    return result;
}

ProtocolFrameDecoder::ProtocolFrameDecoder(usize maxFrameBytes) : maxFrameBytes_(maxFrameBytes) {
    buffer_.reserve(std::min(maxFrameBytes, usize{4096}));
}

ProtocolResult<Vec<ProtocolFrame>> ProtocolFrameDecoder::feed(std::span<const u8> bytes) {
    if (error_) {
        return std::unexpected(*error_);
    }
    if (bytes.size() > maxFrameBytes_ + kFrameLengthPrefixSize ||
        buffer_.size() > maxFrameBytes_ + kFrameLengthPrefixSize -
                             std::min(bytes.size(), maxFrameBytes_ + kFrameLengthPrefixSize)) {
        error_ = resourceError("protocol receive buffer exceeds configured byte limit", buffer_.size());
        return std::unexpected(*error_);
    }
    buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
    Vec<ProtocolFrame> frames;
    while (buffer_.size() >= kFrameLengthPrefixSize) {
        const u32 declared = loadU32(buffer_.data());
        if (declared < kFrameBytesAfterLength) {
            error_ = protocolError("protocol frame length is smaller than its header");
            return std::unexpected(*error_);
        }
        const usize totalBytes = kFrameLengthPrefixSize + static_cast<usize>(declared);
        if (totalBytes > maxFrameBytes_) {
            error_ = resourceError("protocol frame exceeds configured byte limit");
            return std::unexpected(*error_);
        }
        if (buffer_.size() < totalBytes) {
            break;
        }
        auto decoded = decodeProtocolFrame(std::span<const u8>(buffer_.data(), totalBytes), maxFrameBytes_);
        if (!decoded) {
            error_ = decoded.error();
            return std::unexpected(*error_);
        }
        frames.push_back(std::move(*decoded));
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<isize>(totalBytes));
    }
    return frames;
}

ProtocolResult<Vec<ProtocolFrame>> ProtocolFrameDecoder::feed(StrView bytes) {
    return feed(std::span<const u8>(reinterpret_cast<const u8*>(bytes.data()), bytes.size()));
}

ProtocolResult<void> ProtocolFrameDecoder::finish() const {
    if (error_) {
        return std::unexpected(*error_);
    }
    if (!buffer_.empty()) {
        return std::unexpected(protocolError("unexpected EOF inside protocol frame", buffer_.size()));
    }
    return {};
}

void ProtocolFrameDecoder::reset() noexcept {
    buffer_.clear();
    error_.reset();
}

ProtocolResult<Vec<u8>> encodeHello(const ProtocolHello& hello) {
    ProtocolWriter writer;
    writer.writeU16(hello.minimumMajor);
    writer.writeU16(hello.maximumMajor);
    writer.writeU16(hello.minimumMinor);
    writer.writeU16(hello.maximumMinor);
    writer.writeString(hello.clientName);
    writer.writeString(hello.clientVersion);
    writer.writeString(hello.authToken);
    writer.writeU64(hello.requestedCapabilities);
    writer.writeString(hello.stateSelector);
    return std::move(writer).finish();
}

ProtocolResult<ProtocolHello> decodeHello(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    ProtocolHello result;
    auto minimumMajor = reader.readU16();
    auto maximumMajor = reader.readU16();
    auto minimumMinor = reader.readU16();
    auto maximumMinor = reader.readU16();
    auto clientName = reader.readString();
    auto clientVersion = reader.readString();
    auto authToken = reader.readString();
    auto capabilities = reader.readU64();
    auto selector = reader.readString();
    if (!minimumMajor || !maximumMajor || !minimumMinor || !maximumMinor || !clientName || !clientVersion ||
        !authToken || !capabilities || !selector) {
        return std::unexpected(protocolError("malformed protocol hello"));
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    result.minimumMajor = *minimumMajor;
    result.maximumMajor = *maximumMajor;
    result.minimumMinor = *minimumMinor;
    result.maximumMinor = *maximumMinor;
    result.clientName = std::move(*clientName);
    result.clientVersion = std::move(*clientVersion);
    result.authToken = std::move(*authToken);
    result.requestedCapabilities = *capabilities;
    result.stateSelector = std::move(*selector);
    return result;
}

ProtocolResult<Vec<u8>> encodeHelloAck(const ProtocolHelloAck& hello) {
    ProtocolWriter writer;
    writer.writeU16(hello.selectedMajor);
    writer.writeU16(hello.selectedMinor);
    writer.writeU64(hello.capabilities);
    writer.writeU64(hello.sessionId);
    writer.writeU32(hello.heartbeatIntervalMs);
    writer.writeString(hello.serverName);
    writer.writeString(hello.serverVersion);
    return std::move(writer).finish();
}

ProtocolResult<ProtocolHelloAck> decodeHelloAck(std::span<const u8> payload) {
    ProtocolReader reader(payload);
    ProtocolHelloAck result;
    auto major = reader.readU16();
    auto minor = reader.readU16();
    auto capabilities = reader.readU64();
    auto sessionId = reader.readU64();
    auto heartbeat = reader.readU32();
    auto serverName = reader.readString();
    auto serverVersion = reader.readString();
    if (!major || !minor || !capabilities || !sessionId || !heartbeat || !serverName || !serverVersion) {
        return std::unexpected(protocolError("malformed protocol hello acknowledgement"));
    }
    if (auto complete = reader.finish(); !complete) {
        return std::unexpected(complete.error());
    }
    result.selectedMajor = *major;
    result.selectedMinor = *minor;
    result.capabilities = *capabilities;
    result.sessionId = *sessionId;
    result.heartbeatIntervalMs = *heartbeat;
    result.serverName = std::move(*serverName);
    result.serverVersion = std::move(*serverVersion);
    return result;
}

ProtocolResult<ProtocolHelloAck> negotiateHello(const ProtocolHello& hello, u64 availableCapabilities, u64 sessionId,
                                                StrView serverVersion) {
    if (hello.minimumMajor > hello.maximumMajor || hello.minimumMinor > hello.maximumMinor) {
        return std::unexpected(protocolError("invalid protocol version range"));
    }
    if (hello.minimumMajor > kProtocolVersionMajor || hello.maximumMajor < kProtocolVersionMajor) {
        return std::unexpected(
            ProtocolError{ProtocolStatus::VersionMismatch, "no compatible YLDP major version", 0});
    }
    ProtocolHelloAck result;
    result.selectedMajor = kProtocolVersionMajor;
    result.selectedMinor = std::min(hello.maximumMinor, kProtocolVersionMinor);
    if (result.selectedMinor < hello.minimumMinor) {
        return std::unexpected(
            ProtocolError{ProtocolStatus::VersionMismatch, "no compatible YLDP minor version", 0});
    }
    result.capabilities = hello.requestedCapabilities & availableCapabilities;
    result.sessionId = sessionId;
    result.serverVersion = Str(serverVersion);
    return result;
}

} // namespace Lua::Debugger::Remote
