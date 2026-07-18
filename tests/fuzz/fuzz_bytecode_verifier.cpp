#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "core/value.hpp"
#include "runtime/bytecode_verifier.hpp"
#include "runtime/runtime_services.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

void initializeProto(Lua::Proto& proto, std::uint8_t metadata) {
    proto.setMaxStackSize(static_cast<Lua::u8>((metadata % 255U) + 1U));
    proto.setNumParams(static_cast<Lua::u8>((metadata >> 2U) % 8U));
    proto.setNumUpvalues(static_cast<Lua::u8>((metadata >> 5U) % 4U));
    proto.setVarargFlags(static_cast<Lua::u8>(metadata & 7U));
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    constexpr std::size_t kMaxInput = 64 * 1024;
    if (data == nullptr || size > kMaxInput) {
        return 0;
    }

    try {
        Lua::EngineContext context;
        Lua::Proto root(&context.allocator());
        const std::uint8_t metadata = size == 0 ? 0 : data[0];
        initializeProto(root, metadata);

        const std::size_t constantCount = size < 2 ? 0 : data[1] % 32U;
        for (std::size_t index = 0; index < constantCount; ++index) {
            const std::uint8_t byte = data[(index + 2U) % size];
            switch (byte & 3U) {
                case 0:
                    root.appendConstantSlot(Lua::Value());
                    break;
                case 1:
                    root.appendConstantSlot(Lua::Value((byte & 4U) != 0));
                    break;
                default:
                    root.appendConstantSlot(Lua::Value(static_cast<Lua::LuaNumber>(byte)));
                    break;
            }
        }

        const std::size_t codeOffset = std::min<std::size_t>(2, size);
        const std::size_t instructionCount = (size - codeOffset) / sizeof(Lua::Instruction);
        for (std::size_t index = 0; index < instructionCount; ++index) {
            Lua::Instruction instruction = 0;
            std::memcpy(&instruction, data + codeOffset + index * sizeof(instruction), sizeof(instruction));
            root.addInstruction(instruction);
        }
        if (root.getInstructionCount() == 0) {
            root.addInstruction(Lua::CREATE_ABC(Lua::OpCode::RETURN, 0, 1, 0));
        }

        if ((metadata & 0x80U) != 0) {
            Lua::Proto* child = context.gc().create<Lua::Proto>();
            initializeProto(*child, static_cast<std::uint8_t>(metadata ^ 0x5aU));
            child->addInstruction(Lua::CREATE_ABC(Lua::OpCode::RETURN, 0, 1, 0));
            root.addProto(child);
        }

        if ((metadata & 0x40U) != 0) {
            const std::size_t lines = (metadata & 0x20U) != 0 ? root.getInstructionCount()
                                                              : root.getInstructionCount() / 2U;
            for (std::size_t index = 0; index < lines; ++index) {
                root.getLineInfo().push_back(static_cast<Lua::i32>(index));
            }
        }

        Lua::BytecodeVerifierLimits limits;
        limits.maxProtoDepth = 8;
        limits.maxProtoCount = 8;
        limits.maxInstructionCount = 16 * 1024;
        limits.maxConstantCount = 1024;
        limits.maxDebugEntries = 16 * 1024;
        (void)Lua::BytecodeVerifier::verify(root, limits);
    } catch (...) {
        // Allocation and resource-limit failures are valid fuzzer outcomes.
    }
    return 0;
}
