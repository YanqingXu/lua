#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "bytecode_printer.hpp"
#include "io/file_loader.hpp"
#include "runtime/runtime_services.hpp"

#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

using namespace Lua;

namespace {
constexpr const char* kToolName = "bytecode_main";
} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << std::format("Usage: {} <script.lua> [full]\n", kToolName);
        return 1;
    }

    const char* scriptPath = argv[1];
    bool full = (argc >= 3) && (std::string(argv[2]) == "full");

    try {
        Str source = readWholeFile(scriptPath);

        RuntimeServices services = RuntimeServices::fromSingletons();
        Parser parser(source, services);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);

        CodeGenerator codegen(services);
        Proto* proto = codegen.generate(chunk, scriptPath);
        if (!proto) {
            std::cerr << std::format("[ERROR] {}\n", "Failed to generate Proto");
            return 2;
        }

        printProtoBytecode(proto, std::cout, full);

        // Proto由GC管理；字节码工具结束时由进程/GC清理。
        return 0;
    } catch (const std::exception& e) {
        std::cerr << std::format("[ERROR] Exception: {}\n", e.what());
        return 3;
    } catch (...) {
        std::cerr << std::format("[ERROR] {}\n", "Unknown exception");
        return 4;
    }
}

