#include "lauxlib.h"
#include "lua.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    constexpr std::size_t kMaxInput = 1024 * 1024;
    if (data == nullptr || size > kMaxInput) {
        return 0;
    }

    lua_State* state = nullptr;
    try {
        state = lua_newstate(nullptr, nullptr);
        if (state == nullptr) {
            return 0;
        }

        constexpr char kSignature[] = {'\x1b', 'L', 'u', 'a'};
        const bool alreadyBinary = size >= sizeof(kSignature) &&
                                   std::memcmp(data, kSignature, sizeof(kSignature)) == 0;
        std::vector<char> chunk;
        const char* bytes = reinterpret_cast<const char*>(data);
        std::size_t byteCount = size;
        if (!alreadyBinary) {
            chunk.insert(chunk.end(), kSignature, kSignature + sizeof(kSignature));
            chunk.insert(chunk.end(), bytes, bytes + size);
            bytes = chunk.data();
            byteCount = chunk.size();
        }

        (void)luaL_loadbuffer(state, bytes, byteCount, "=fuzz_undump");
        lua_settop(state, 0);
    } catch (...) {
        // The public load boundary should normally translate failures to a Lua
        // status. Keep the harness exception-safe so teardown is still tested.
    }

    lua_close(state);
    return 0;
}
