#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "bytecode_printer.hpp"
#include "core/string_pool.hpp"

#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace Lua;

static std::string readFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error(std::string("Cannot open file: ") + path);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: bytecode_main <script.lua> [full]\n";
        return 1;
    }

    const char* scriptPath = argv[1];
    bool full = (argc >= 3) && (std::string(argv[2]) == "full");

    try {
        std::string source = readFile(scriptPath);

        StringPool& pool = StringPool::getInstance();
        Parser parser(source);
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk, scriptPath);
        if (!proto) {
            std::cerr << "[ERROR] Failed to generate Proto" << std::endl;
            return 2;
        }

        printProtoBytecode(proto, std::cout, full);

        // Proto由GC管理；字节码工具结束时由进程/GC清理。
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
        return 3;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception" << std::endl;
        return 4;
    }
}

