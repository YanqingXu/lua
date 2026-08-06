#include "debugger/remote_protocol.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

using namespace Lua;
using namespace Lua::Debugger::Remote;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const std::span<const u8> input(data, size);
    (void)decodeProtocolFrame(input);
    (void)decodeHello(input);
    (void)decodeHelloAck(input);

    ProtocolFrameDecoder decoder;
    usize offset = 0;
    while (offset < input.size()) {
        const usize chunk = std::min<usize>(input.size() - offset, 1U + (input[offset] % 31U));
        if (!decoder.feed(input.subspan(offset, chunk))) {
            break;
        }
        offset += chunk;
    }
    (void)decoder.finish();

    ProtocolReader reader(input);
    (void)reader.readU8();
    (void)reader.readU16();
    (void)reader.readU32();
    (void)reader.readU64();
    (void)reader.readBool();
    (void)reader.readString();
    (void)reader.readBytes();
    (void)reader.finish();
    return 0;
}
