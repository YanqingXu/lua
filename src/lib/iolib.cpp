/**
 * @file iolib.cpp
 * @brief Lua I/O库实现
 * 
 * 使用现代C++流式API进行函数注册
 * 遵循Lua 5.1.5标准I/O库规范
 * 
 * @author Lua C++ Project
 * @date 2025-12-19
 */

#include "lib/iolib.hpp"
#include "lib/lib_registry.hpp"
#include "lib/lib_manager.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/userdata.hpp"
#include "vm/global_state.hpp"
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <string>

namespace Lua {

// =====================================================================
// 常量定义
// =====================================================================

static const char* IO_INPUT = "io.input";
static const char* IO_OUTPUT = "io.output";
static const char* FILE_HANDLE_METATABLE = "FILE*";

// =====================================================================
// 辅助函数实现
// =====================================================================

static std::string errnoMessage(int err) {
#ifdef _MSC_VER
    char buf[256] = {};
    strerror_s(buf, sizeof(buf), err);
    return std::string(buf);
#else
    return std::string(std::strerror(err));
#endif
}

static FILE* safeFopen(const char* filename, const char* mode) {
#ifdef _MSC_VER
    FILE* fp = nullptr;
    if (fopen_s(&fp, filename, mode) != 0) {
        return nullptr;
    }
    return fp;
#else
    return std::fopen(filename, mode);
#endif
}

static FILE* safeTmpfile() {
#ifdef _MSC_VER
    FILE* fp = nullptr;
    if (tmpfile_s(&fp) != 0) {
        return nullptr;
    }
    return fp;
#else
    return std::tmpfile();
#endif
}

/**
 * @brief 推送操作结果到栈
 * 
 * 成功时推送 true，失败时推送 nil、错误消息、错误码
 */
static i32 pushResult(LuaState* L, bool success, const char* filename = nullptr) {
    if (success) {
        L->pushBoolean(true);
        return 1;
    } else {
        i32 err = errno;
        L->pushNil();
        
        // 构造错误消息
        std::string msg;
        if (filename) {
            msg = std::string(filename) + ": " + errnoMessage(err);
        } else {
            msg = errnoMessage(err);
        }
        
        GCString* errMsg = L->getGlobalState().getStringPool().intern(msg.c_str());
        L->pushString(errMsg);
        L->pushNumber(static_cast<f64>(err));
        return 3;
    }
}

/**
 * @brief 推送文件错误并抛出异常
 */
[[noreturn]] static void fileError(LuaState* L, i32 arg, const char* filename) {
    (void)arg;

    i32 err = errno;
    std::string msg = std::string(filename) + ": " + errnoMessage(err);
    L->error(msg.c_str());
}

FILE** toFilePtr(LuaState* L, i32 idx) {
    // 检查是否为 userdata
    if (!L->isUserdata(idx)) {
        return nullptr;
    }
    
    Value val = L->at(idx);
    if (!val.isUserdata()) {
        return nullptr;
    }
    
    Userdata* ud = val.asUserdata();
    if (ud->getDataSize() != sizeof(FILE*)) {
        return nullptr;
    }
    
    return static_cast<FILE**>(ud->getData());
}

FILE** checkFilePtr(LuaState* L, i32 idx) {
    FILE** fp = toFilePtr(L, idx);
    if (!fp) {
        L->error("bad argument (FILE* expected)");
    }
    return fp;
}

Userdata* createFileHandle(LuaState* L, FILE* fp) {
    // 创建 userdata
    Userdata* ud = Userdata::createFull(sizeof(FILE*));
    L->getGlobalState().getGC().registerObject(ud);
    
    // 设置文件指针
    FILE** pf = static_cast<FILE**>(ud->getData());
    *pf = fp;
    
    // 获取文件元表
    GCString* mtName = L->getGlobalState().getStringPool().intern(FILE_HANDLE_METATABLE);
    Value mtVal = L->getGlobal(mtName->getData());
    if (mtVal.isTable()) {
        ud->setMetatable(mtVal.asTable());
    }
    
    return ud;
}

FILE* getDefaultInput(LuaState* L) {
    GCString* key = L->getGlobalState().getStringPool().intern(IO_INPUT);
    Value val = L->getGlobal(key->getData());
    
    if (val.isUserdata()) {
        FILE** fp = toFilePtr(L, L->getTop());
        L->pop();

        if (fp && *fp) {
            return *fp;
        }
    }
    
    return stdin;
}

FILE* getDefaultOutput(LuaState* L) {
    GCString* key = L->getGlobalState().getStringPool().intern(IO_OUTPUT);
    Value val = L->getGlobal(key->getData());
    
    if (val.isUserdata()) {
        FILE** fp = toFilePtr(L, L->getTop());
        L->pop();

        if (fp && *fp) {
            return *fp;
        }
    }
    
    return stdout;
}

void setDefaultInput(LuaState* L, FILE* fp) {
    Userdata* ud = createFileHandle(L, fp);
    L->setGlobal(IO_INPUT, Value(ud));
}

void setDefaultOutput(LuaState* L, FILE* fp) {
    Userdata* ud = createFileHandle(L, fp);
    L->setGlobal(IO_OUTPUT, Value(ud));
}

// =====================================================================
// 读取辅助函数
// =====================================================================

/**
 * @brief 读取一行（不包括换行符）
 */
static bool readLine(LuaState* L, FILE* fp) {
    std::string line;
    i32 c;
    
    while ((c = std::fgetc(fp)) != EOF && c != '\n') {
        line += static_cast<char>(c);
    }
    
    if (line.empty() && c == EOF) {
        return false;  // EOF
    }
    
    GCString* str = L->getGlobalState().getStringPool().intern(line.c_str());
    L->pushString(str);
    return true;
}

/**
 * @brief 读取指定字符数
 */
static bool readChars(LuaState* L, FILE* fp, usize count) {
    std::string buffer;
    buffer.reserve(count);
    
    for (usize i = 0; i < count; i++) {
        i32 c = std::fgetc(fp);
        if (c == EOF) {
            if (i == 0) {
                return false;  // EOF at start
            }
            break;
        }
        buffer += static_cast<char>(c);
    }
    
    GCString* str = L->getGlobalState().getStringPool().intern(buffer.c_str());
    L->pushString(str);
    return true;
}

/**
 * @brief 读取整个文件
 */
static bool readAll(LuaState* L, FILE* fp) {
    std::string content;
    i32 c;
    
    while ((c = std::fgetc(fp)) != EOF) {
        content += static_cast<char>(c);
    }
    
    GCString* str = L->getGlobalState().getStringPool().intern(content.c_str());
    L->pushString(str);
    return true;
}

/**
 * @brief 读取一个数字
 */
static bool readNumber(LuaState* L, FILE* fp) {
    char buf[256];
    if (!std::fgets(buf, static_cast<int>(sizeof(buf)), fp)) {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    double v = std::strtod(buf, &end);
    if (end == buf) {
        return false;
    }

    L->pushNumber(static_cast<f64>(v));
    return true;
}

// =====================================================================
// I/O库函数实现
// =====================================================================

i32 io_open(LuaState* L) {
    // 检查参数
    if (L->getTop() < 1) {
        L->error("io.open: filename expected");
    }

    if (!L->isString(1)) {
        L->error("io.open: filename must be a string");
    }

    const char* filename = L->toString(1);
    const char* mode = L->getTop() >= 2 && L->isString(2) ? L->toString(2) : "r";

    // 打开文件
    FILE* fp = safeFopen(filename, mode);
    if (!fp) {
        return pushResult(L, false, filename);
    }

    // 创建文件句柄
    Userdata* ud = createFileHandle(L, fp);
    L->pushUserdata(ud);
    return 1;
}

i32 io_close(LuaState* L) {
    FILE** fp;
    
    if (L->getTop() == 0) {
        // 关闭默认输出
        fp = toFilePtr(L, -1);  // 这里需要从全局环境获取
        if (!fp) {
            L->error("io.close: no default output file");
        }
    } else {
        fp = checkFilePtr(L, 1);
    }
    
    if (!*fp) {
        L->pushNil();
        GCString* msg = L->getGlobalState().getStringPool().intern("file is already closed");
        L->pushString(msg);
        return 2;
    }
    
    i32 result = std::fclose(*fp);
    *fp = nullptr;
    
    return pushResult(L, result == 0);
}

// 前向声明
static i32 f_read_impl(LuaState* L, FILE* fp, i32 firstArg);
static i32 f_write_impl(LuaState* L, FILE* fp);

i32 io_read(LuaState* L) {
    FILE* fp = getDefaultInput(L);
    return f_read_impl(L, fp, 1);
}

i32 io_write(LuaState* L) {
    FILE* fp = getDefaultOutput(L);
    return f_write_impl(L, fp);
}

i32 io_flush(LuaState* L) {
    FILE* fp = getDefaultOutput(L);
    return pushResult(L, std::fflush(fp) == 0);
}

i32 io_input(LuaState* L) {
    if (L->getTop() == 0) {
        // 返回当前输入文件
        FILE* fp = getDefaultInput(L);
        Userdata* ud = createFileHandle(L, fp);
        L->pushUserdata(ud);
        return 1;
    } else {
        // 设置新的输入文件
        if (L->isString(1)) {
            // 打开文件
            const char* filename = L->toString(1);
            FILE* fp = safeFopen(filename, "r");
            if (!fp) {
                fileError(L, 1, filename);
            }
            setDefaultInput(L, fp);
        } else {
            // 使用提供的文件句柄
            FILE** fp = checkFilePtr(L, 1);
            setDefaultInput(L, *fp);
        }
        L->pushValue(1);
        return 1;
    }
}

i32 io_output(LuaState* L) {
    if (L->getTop() == 0) {
        // 返回当前输出文件
        FILE* fp = getDefaultOutput(L);
        Userdata* ud = createFileHandle(L, fp);
        L->pushUserdata(ud);
        return 1;
    } else {
        // 设置新的输出文件
        if (L->isString(1)) {
            // 打开文件
            const char* filename = L->toString(1);
            FILE* fp = safeFopen(filename, "w");
            if (!fp) {
                fileError(L, 1, filename);
            }
            setDefaultOutput(L, fp);
        } else {
            // 使用提供的文件句柄
            FILE** fp = checkFilePtr(L, 1);
            setDefaultOutput(L, *fp);
        }
        L->pushValue(1);
        return 1;
    }
}

i32 io_type(LuaState* L) {
    if (L->getTop() < 1) {
        L->pushNil();
        return 1;
    }
    
    FILE** fp = toFilePtr(L, 1);
    if (!fp) {
        L->pushNil();
        return 1;
    }
    
    GCString* typeStr;
    if (*fp) {
        typeStr = L->getGlobalState().getStringPool().intern("file");
    } else {
        typeStr = L->getGlobalState().getStringPool().intern("closed file");
    }
    
    L->pushString(typeStr);
    return 1;
}

/**
 * @brief 行迭代器函数
 *
 * 这是一个C函数，用作迭代器。每次调用时读取文件的下一行。
 * 使用upvalue存储文件句柄。
 */
static i32 lines_iterator(LuaState* L) {
    // 从upvalue获取文件句柄
    // 简化实现：由于当前没有完整的upvalue支持，我们使用全局状态
    // TODO: 使用upvalue存储文件句柄

    // 暂时返回错误
    L->error("io.lines iterator: upvalue support needed");
    return 0;
}

i32 io_lines(LuaState* L) {
    FILE* fp = nullptr;
    bool shouldClose = false;

    if (L->getTop() == 0) {
        // 无参数：使用默认输入
        fp = getDefaultInput(L);
        shouldClose = false;
    } else if (L->isString(1)) {
        // 字符串参数：打开文件
        const char* filename = L->toString(1);
        fp = safeFopen(filename, "r");
        if (!fp) {
            fileError(L, 1, filename);
        }
        shouldClose = true;
    } else {
        L->error("io.lines: string expected");
    }

    // 简化实现：由于缺少完整的闭包/upvalue支持，
    // 我们返回一个错误提示
    // TODO: 创建带有upvalue的迭代器闭包
    L->error("io.lines: iterator closures not yet fully supported");
    return 0;
}

i32 io_tmpfile(LuaState* L) {
    FILE* fp = safeTmpfile();
    if (!fp) {
        return pushResult(L, false);
    }

    Userdata* ud = createFileHandle(L, fp);
    L->pushUserdata(ud);
    return 1;
}

i32 io_popen(LuaState* L) {
    // 检查参数
    if (L->getTop() < 1) {
        L->error("io.popen: command expected");
    }

    if (!L->isString(1)) {
        L->error("io.popen: command must be a string");
    }

    const char* command = L->toString(1);
    const char* mode = L->getTop() >= 2 && L->isString(2) ? L->toString(2) : "r";

    // 验证模式
    if (std::strcmp(mode, "r") != 0 && std::strcmp(mode, "w") != 0) {
        L->error("io.popen: invalid mode (must be 'r' or 'w')");
    }

    // 打开管道
#ifdef _WIN32
    FILE* fp = _popen(command, mode);
#else
    FILE* fp = popen(command, mode);
#endif

    if (!fp) {
        return pushResult(L, false, command);
    }

    // 创建文件句柄
    Userdata* ud = createFileHandle(L, fp);
    L->pushUserdata(ud);
    return 1;
}

// =====================================================================
// 文件句柄方法实现
// =====================================================================

/**
 * @brief 实际的读取实现
 */
static i32 f_read_impl(LuaState* L, FILE* fp, i32 firstArg) {
    i32 nargs = L->getTop() - firstArg + 1;
    i32 n = 0;
    bool success = true;
    
    if (nargs == 0) {
        // 默认读取一行
        success = readLine(L, fp);
        n = success ? 1 : 0;
    } else {
        // 处理每个参数
        for (i32 i = firstArg; i <= L->getTop(); i++) {
            if (L->isNumber(i)) {
                // 读取指定字符数
                f64 num = L->toNumber(i);
                if (num >= 0) {
                    success = readChars(L, fp, static_cast<usize>(num));
                } else {
                    success = false;
                }
            } else if (L->isString(i)) {
                // 读取格式
                const char* fmt = L->toString(i);
                if (std::strcmp(fmt, "*n") == 0) {
                    success = readNumber(L, fp);
                } else if (std::strcmp(fmt, "*a") == 0) {
                    success = readAll(L, fp);
                } else if (std::strcmp(fmt, "*l") == 0) {
                    success = readLine(L, fp);
                } else {
                    L->error("invalid format");
                }
            } else {
                L->error("invalid argument to read");
            }
            
            if (success) {
                n++;
            } else {
                L->pushNil();
                n++;
                break;
            }
        }
    }
    
    return n;
}

/**
 * @brief 实际的写入实现
 */
static i32 f_write_impl(LuaState* L, FILE* fp) {
    i32 nargs = L->getTop();
    bool success = true;
    
    for (i32 i = 1; i <= nargs; i++) {
        if (L->isString(i)) {
            const char* str = L->toString(i);
            usize len = std::strlen(str);
            if (std::fwrite(str, 1, len, fp) != len) {
                success = false;
                break;
            }
        } else if (L->isNumber(i)) {
            f64 num = L->toNumber(i);
            if (std::fprintf(fp, "%.14g", num) < 0) {
                success = false;
                break;
            }
        } else {
            L->error("invalid argument to write");
        }
    }
    
    return pushResult(L, success);
}

i32 f_close(LuaState* L) {
    FILE** fp = checkFilePtr(L, 1);
    
    if (!*fp) {
        L->pushNil();
        GCString* msg = L->getGlobalState().getStringPool().intern("file is already closed");
        L->pushString(msg);
        return 2;
    }
    
    i32 result = std::fclose(*fp);
    *fp = nullptr;
    
    return pushResult(L, result == 0);
}

i32 f_read(LuaState* L) {
    FILE** fp = checkFilePtr(L, 1);
    if (!*fp) {
        L->error("attempt to use a closed file");
    }
    return f_read_impl(L, *fp, 2);
}

i32 f_write(LuaState* L) {
    FILE** fp = checkFilePtr(L, 1);
    if (!*fp) {
        L->error("attempt to use a closed file");
    }
    
    // 移除第一个参数（文件句柄）后再写入
    i32 nargs = L->getTop() - 1;
    bool success = true;
    
    for (i32 i = 2; i <= L->getTop(); i++) {
        if (L->isString(i)) {
            const char* str = L->toString(i);
            usize len = std::strlen(str);
            if (std::fwrite(str, 1, len, *fp) != len) {
                success = false;
                break;
            }
        } else if (L->isNumber(i)) {
            f64 num = L->toNumber(i);
            if (std::fprintf(*fp, "%.14g", num) < 0) {
                success = false;
                break;
            }
        } else {
            L->error("invalid argument to write");
        }
    }
    
    return pushResult(L, success);
}

i32 f_flush(LuaState* L) {
    FILE** fp = checkFilePtr(L, 1);
    if (!*fp) {
        L->error("attempt to use a closed file");
    }
    return pushResult(L, std::fflush(*fp) == 0);
}

i32 f_seek(LuaState* L) {
    FILE** fp = checkFilePtr(L, 1);
    if (!*fp) {
        L->error("attempt to use a closed file");
    }
    
    // 获取 whence 参数
    i32 whence = SEEK_CUR;  // 默认
    if (L->getTop() >= 2 && L->isString(2)) {
        const char* w = L->toString(2);
        if (std::strcmp(w, "set") == 0) {
            whence = SEEK_SET;
        } else if (std::strcmp(w, "cur") == 0) {
            whence = SEEK_CUR;
        } else if (std::strcmp(w, "end") == 0) {
            whence = SEEK_END;
        } else {
            L->error("invalid seek mode");
        }
    }
    
    // 获取 offset 参数
    long offset = 0;
    if (L->getTop() >= 3 && L->isNumber(3)) {
        offset = static_cast<long>(L->toNumber(3));
    }
    
    // 执行 seek
    if (std::fseek(*fp, offset, whence) != 0) {
        return pushResult(L, false);
    }
    
    // 返回新位置
    long pos = std::ftell(*fp);
    L->pushNumber(static_cast<f64>(pos));
    return 1;
}

i32 f_setvbuf(LuaState* L) {
    FILE** fp = checkFilePtr(L, 1);
    if (!*fp) {
        L->error("attempt to use a closed file");
    }
    
    if (L->getTop() < 2 || !L->isString(2)) {
        L->error("string expected");
    }
    
    const char* mode = L->toString(2);
    i32 m;
    
    if (std::strcmp(mode, "no") == 0) {
        m = _IONBF;
    } else if (std::strcmp(mode, "full") == 0) {
        m = _IOFBF;
    } else if (std::strcmp(mode, "line") == 0) {
        m = _IOLBF;
    } else {
        L->error("invalid buffer mode");
        return 0;
    }
    
    usize size = BUFSIZ;
    if (L->getTop() >= 3 && L->isNumber(3)) {
        size = static_cast<usize>(L->toNumber(3));
    }
    
    i32 result = std::setvbuf(*fp, nullptr, m, size);
    return pushResult(L, result == 0);
}

i32 f_lines(LuaState* L) {
    FILE** fp = checkFilePtr(L, 1);
    if (!*fp) {
        L->error("attempt to use a closed file");
    }

    // 简化实现：由于缺少完整的闭包/upvalue支持，
    // 我们返回一个错误提示
    // TODO: 创建带有upvalue的迭代器闭包
    L->error("file:lines: iterator closures not yet fully supported");
    return 0;
}

// =====================================================================
// 元表方法实现
// =====================================================================

i32 io_gc(LuaState* L) {
    FILE** fp = toFilePtr(L, 1);
    if (fp && *fp) {
        std::fclose(*fp);
        *fp = nullptr;
    }
    return 0;
}

i32 io_tostring(LuaState* L) {
    FILE** fp = toFilePtr(L, 1);
    if (!fp) {
        L->pushString(L->getGlobalState().getStringPool().intern("not a file"));
        return 1;
    }
    
    std::string str;
    if (*fp) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "file (%p)", static_cast<void*>(*fp));
        str = buffer;
    } else {
        str = "file (closed)";
    }
    
    GCString* result = L->getGlobalState().getStringPool().intern(str.c_str());
    L->pushString(result);
    return 1;
}

// =====================================================================
// 库注册和初始化
// =====================================================================

void IOLibModule::registerFunctions(LuaState* L) {
    if (!L) {
        return;
    }

    // 创建 io 表
    Table* ioTable = FunctionRegistrar::createLibTable(L, "io");
    if (!ioTable) {
        L->error("Failed to create io library table");
        return;
    }

    // 注册 I/O 库函数
    FunctionRegistrar(L)
        .addGlobal("open", io_open)
        .addGlobal("close", io_close)
        .addGlobal("read", io_read)
        .addGlobal("write", io_write)
        .addGlobal("flush", io_flush)
        .addGlobal("input", io_input)
        .addGlobal("output", io_output)
        .addGlobal("type", io_type)
        .addGlobal("lines", io_lines)
        .addGlobal("tmpfile", io_tmpfile)
        .addGlobal("popen", io_popen)
        .commitToTable(ioTable);
    
    // 创建文件句柄元表
    Table* fileMT = new Table();
    L->getGlobalState().getGC().registerObject(fileMT);
    
    // 注册文件方法
    FunctionRegistrar(L)
        .addGlobal("close", f_close)
        .addGlobal("read", f_read)
        .addGlobal("write", f_write)
        .addGlobal("flush", f_flush)
        .addGlobal("seek", f_seek)
        .addGlobal("setvbuf", f_setvbuf)
        .addGlobal("lines", f_lines)
        .commitToTable(fileMT);
    
    // 设置元方法
    GCString* gcKey = L->getGlobalState().getStringPool().intern("__gc");
    GCString* tostringKey = L->getGlobalState().getStringPool().intern("__tostring");
    GCString* indexKey = L->getGlobalState().getStringPool().intern("__index");
    
    // 创建 __gc 和 __tostring 函数
    // 注意：这里需要创建 Function 对象
    // TODO: 完整实现
    
    // __index 指向自身
    fileMT->set(Value(indexKey), Value(fileMT));
    
    // 保存文件元表到全局
    GCString* mtName = L->getGlobalState().getStringPool().intern(FILE_HANDLE_METATABLE);
    L->setGlobal(mtName->getData(), Value(fileMT));
}

void IOLibModule::initialize(LuaState* L) {
    if (!L) {
        return;
    }

    // 创建标准文件句柄
    Userdata* stdinHandle = createFileHandle(L, stdin);
    Userdata* stdoutHandle = createFileHandle(L, stdout);
    Userdata* stderrHandle = createFileHandle(L, stderr);
    
    // 设置到 io 表中
    auto& gs = L->getGlobalState();
    GCString* ioKey = gs.getStringPool().intern("io");
    Value ioTableVal = L->getGlobal(ioKey->getData());
    
    if (ioTableVal.isTable()) {
        Table* ioTable = ioTableVal.asTable();
        
        GCString* stdinKey = gs.getStringPool().intern("stdin");
        GCString* stdoutKey = gs.getStringPool().intern("stdout");
        GCString* stderrKey = gs.getStringPool().intern("stderr");
        
        ioTable->set(Value(stdinKey), Value(stdinHandle));
        ioTable->set(Value(stdoutKey), Value(stdoutHandle));
        ioTable->set(Value(stderrKey), Value(stderrHandle));
    }
    
    // 设置默认输入输出
    setDefaultInput(L, stdin);
    setDefaultOutput(L, stdout);
}

void openIOLib(LuaState* L) {
    if (!L) {
        return;
    }

    IOLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
