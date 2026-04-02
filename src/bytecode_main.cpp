#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "compiler/bytecode_printer.hpp"
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
        Proto* proto = codegen.generate(chunk);
        if (!proto) {
            std::cerr << "[ERROR] Failed to generate Proto" << std::endl;
            return 2;
        }

        // 为了让输出看起来更接近 luaU_print，这里把源文件名写入 Proto::source
        GCString* sourceName = pool.intern(scriptPath, static_cast<usize>(std::strlen(scriptPath)));

        // 递归设置所有 Proto 的源文件名
        std::function<void(Proto*)> setSourceRecursive = [&](Proto* p) {
            p->setSource(sourceName);
            for (usize i = 0; i < p->getSubProtoCount(); ++i) {
                setSourceRecursive(p->getSubProto(i));
            }
        };
        setSourceRecursive(proto);

        printProtoBytecode(proto, std::cout, full);

        delete proto;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
        return 3;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception" << std::endl;
        return 4;
    }
}

