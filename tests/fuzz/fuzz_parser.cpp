#include "common/types.hpp"
#include "compiler/parser/parser.hpp"
#include "runtime/runtime_services.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    constexpr Lua::usize kMaxInput = 1024 * 1024;
    if (data == nullptr || size > kMaxInput) {
        return 0;
    }

    try {
        Lua::EngineContext context;
        context.resourcePolicy().maxSourceBytes = kMaxInput;
        context.compilationPolicy().maxSourceBytes = kMaxInput;
        Lua::RuntimeServices services = context.services();
        Lua::Str source(reinterpret_cast<Lua::CharPtr>(data), size);
        Lua::Parser parser(source, services, Lua::ParserOptions{Lua::ParseRecoveryMode::StatementBoundary});
        (void)parser.parse();
    } catch (...) {
        // Syntax, resource, and allocation errors are expected outcomes.
    }
    return 0;
}
