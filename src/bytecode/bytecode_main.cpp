/**
 * @file bytecode_main.cpp
 * @brief 字节码查看与差异比较命令行工具入口
 */

#include "common/types.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include "bytecode_printer.hpp"
#include "io/file_loader.hpp"
#include "runtime/runtime_services.hpp"

#include <expected>
#include <format>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace Lua;

namespace {
constexpr Lua::CharPtr kToolName = "bytecode_main";

struct BytecodeToolOptions {
    bool diff = false;
    bool full = false;
    bool cfg = false;
    Lua::Vec<Lua::Str> scripts;
};

void printUsage(std::ostream& err) {
    err << "Usage:\n";
    err << std::format("  {} <script.lua> [full|--full]\n", kToolName);
    err << std::format("  {} <script.lua> --cfg [full|--full]\n", kToolName);
    err << std::format("  {} <left.lua> <right.lua> --diff [full|--full]\n", kToolName);
}

Lua::Str usageText() {
    std::ostringstream out;
    printUsage(out);
    return out.str();
}

Lua::Unexpect<Lua::Str> optionError(Lua::Str message) {
    if (!message.empty() && message.back() != '\n') {
        message.push_back('\n');
    }
    message += usageText();
    return Lua::Unexpect<Lua::Str>(std::move(message));
}

Lua::Expect<BytecodeToolOptions, Lua::Str> parseOptions(int argc, char** argv) {
    if (argc < 2) {
        return Lua::Unexpect<Lua::Str>(usageText());
    }

    BytecodeToolOptions options;
    for (int i = 1; i < argc; ++i) {
        Lua::Str arg = argv[i];
        if (arg == "--diff") {
            options.diff = true;
        } else if (arg == "--cfg") {
            options.cfg = true;
        } else if (arg == "full" || arg == "--full") {
            options.full = true;
        } else if (!arg.empty() && arg[0] == '-') {
            return optionError(std::format("[ERROR] Unknown option: {}", arg));
        } else {
            options.scripts.push_back(std::move(arg));
        }
    }

    if (options.diff && options.cfg) {
        return optionError("[ERROR] --cfg cannot be combined with --diff");
    }

    const Lua::usize expectedScripts = options.diff ? 2 : 1;
    if (options.scripts.size() != expectedScripts) {
        return optionError(std::format(
            "[ERROR] Expected {} script path{} for {} mode, got {}", expectedScripts, expectedScripts == 1 ? "" : "s",
            options.diff ? "diff" : (options.cfg ? "cfg" : "print"), options.scripts.size()));
    }

    return options;
}

Proto* compileScript(RuntimeServices& services, const Lua::Str& scriptPath) {
    Lua::Str source = readWholeFile(scriptPath);

    Parser parser(source, services);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(services);
    auto generated = codegen.tryGenerate(chunk, scriptPath);
    if (!generated) {
        throw generated.error();
    }

    return *generated;
}

} // namespace

int main(int argc, char** argv) {
    auto parsedOptions = parseOptions(argc, argv);
    if (!parsedOptions) {
        std::cerr << parsedOptions.error();
        return 1;
    }
    BytecodeToolOptions options = std::move(*parsedOptions);

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
