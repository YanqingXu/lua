#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include "bytecode_printer.hpp"
#include "io/file_loader.hpp"
#include "runtime/runtime_services.hpp"

#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace Lua;

namespace {
constexpr const char* kToolName = "bytecode_main";

struct BytecodeToolOptions {
    bool diff = false;
    bool full = false;
    bool cfg = false;
    std::vector<std::string> scripts;
};

void printUsage(std::ostream& err) {
    err << "Usage:\n";
    err << std::format("  {} <script.lua> [full|--full]\n", kToolName);
    err << std::format("  {} <script.lua> --cfg [full|--full]\n", kToolName);
    err << std::format("  {} <left.lua> <right.lua> --diff [full|--full]\n", kToolName);
}

bool parseOptions(int argc, char** argv, BytecodeToolOptions& options, std::ostream& err) {
    if (argc < 2) {
        printUsage(err);
        return false;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--diff") {
            options.diff = true;
        } else if (arg == "--cfg") {
            options.cfg = true;
        } else if (arg == "full" || arg == "--full") {
            options.full = true;
        } else if (!arg.empty() && arg[0] == '-') {
            err << std::format("[ERROR] Unknown option: {}\n", arg);
            printUsage(err);
            return false;
        } else {
            options.scripts.push_back(std::move(arg));
        }
    }

    if (options.diff && options.cfg) {
        err << "[ERROR] --cfg cannot be combined with --diff\n";
        printUsage(err);
        return false;
    }

    const usize expectedScripts = options.diff ? 2 : 1;
    if (options.scripts.size() != expectedScripts) {
        err << std::format("[ERROR] Expected {} script path{} for {} mode, got {}\n",
                           expectedScripts,
                           expectedScripts == 1 ? "" : "s",
                           options.diff ? "diff" : (options.cfg ? "cfg" : "print"),
                           options.scripts.size());
        printUsage(err);
        return false;
    }

    return true;
}

Proto* compileScript(RuntimeServices& services, const std::string& scriptPath) {
    Str source = readWholeFile(scriptPath);

    Parser parser(source, services);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(services);
    Proto* proto = codegen.generate(chunk, scriptPath);
    if (!proto) {
        throw std::runtime_error(std::format("Failed to generate Proto for {}", scriptPath));
    }

    return proto;
}

} // namespace

int main(int argc, char** argv) {
    BytecodeToolOptions options;
    if (!parseOptions(argc, argv, options, std::cerr)) {
        return 1;
    }

    try {
        EngineContext engine;
        RuntimeServices services = engine.services();

        if (options.diff) {
            Proto* left = compileScript(services, options.scripts[0]);
            Proto* right = compileScript(services, options.scripts[1]);
            printProtoBytecodeDiff(left, right, std::cout, options.full, options.scripts[0], options.scripts[1]);
        } else {
            Proto* proto = compileScript(services, options.scripts[0]);
            if (options.cfg) {
                printProtoBytecodeCfg(proto, std::cout, options.full);
            } else {
                printProtoBytecode(proto, std::cout, options.full);
            }
        }

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

