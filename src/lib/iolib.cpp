/**
 * @file iolib.cpp
 * @brief Lua I/O库实现
 *
 * 使用现代C++流式API进行函数注册
 * 遵循Lua 5.1.5标准I/O库规范
 *
 * @author Lua C++ 项目
 * @date 2025-12-19
 */

#include "lib/iolib.hpp"
#include "lib/lib_registry.hpp"
#include "lib/lib_manager.hpp"
#include "common/number_conversion.hpp"
#include "core/gc_string.hpp"
#include "core/function.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "runtime/lua_allocator.hpp"
#include "vm/state/global_state.hpp"
#include <cstdio>
#include <expected>
#include <format>
#include <cctype>
#include <cstring>
#include <cerrno>
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>

#ifdef _WIN32
#include <share.h>
#endif

namespace Lua {

static i32 checkedIOInteger(LuaState* L, i32 index, const char* message) {
    if (!L->isNumber(index)) {
        L->error(message);
    }
    const auto converted = checkedLuaInteger(L->toNumber(index));
    if (!converted) {
        L->error(message);
    }
    return *converted;
}

// =====================================================================
// 常量定义
// =====================================================================

static constexpr const char* IO_INPUT = "io.input";
static constexpr const char* IO_OUTPUT = "io.output";
static constexpr StrView FILE_HANDLE_METATABLE = "FILE*";

struct FileCloser {
    bool isPipe = false;
    bool ownsFile = true;

    static i32 close(FILE* fp, bool pipe) noexcept {
        if (fp == nullptr) {
            return 0;
        }

        if (pipe) {
#ifdef _WIN32
            return _pclose(fp);
#else
            return pclose(fp);
#endif
        }

        return std::fclose(fp);
    }

    void operator()(FILE* fp) const noexcept {
        if (!ownsFile) {
            return;
        }

        [[maybe_unused]] const i32 result = close(fp, isPipe);
    }
};

struct FileHandleData;
static void unregisterFileHandle(FileHandleData* handle) noexcept;

struct FileHandleData {
    using FilePtr = std::unique_ptr<FILE, FileCloser>;

    FilePtr file{nullptr, FileCloser{}};
    bool lineBuffered = false;
    Str path;
    GlobalState* owner = nullptr;

    ~FileHandleData() {
        unregisterFileHandle(this);
    }

    FILE* get() const noexcept {
        return file.get();
    }

    void reset(FILE* fp, bool pipe, bool ownsFile) noexcept {
        file = FilePtr(fp, FileCloser{pipe, ownsFile});
    }

    i32 close() noexcept {
        const FileCloser closer = file.get_deleter();
        FILE* fp = file.release();
        if (!closer.ownsFile) {
            return 0;
        }
        return FileCloser::close(fp, closer.isPipe);
    }
};

static i32 lines_iterator(LuaState* L);
static FileHandleData* toFileHandle(const Value& val);
static i32 closeFileHandle(FileHandleData* handle);
struct OpenFileHandle {
    GlobalState* owner = nullptr;
    FileHandleData* handle = nullptr;
};

struct OpenFileRegistry {
    std::mutex mutex;
    Vec<OpenFileHandle> handles;
};

static OpenFileRegistry& openFileRegistry() {
    /**
     * @brief 某些嵌入路径中的运行时状态是进程生命周期单例，因此让注册表存活到静态析构结束。
     */
    static auto* registry = new OpenFileRegistry();
    return *registry;
}

static void unregisterFileHandle(FileHandleData* handle) noexcept {
    try {
        OpenFileRegistry& registry = openFileRegistry();
        std::lock_guard lock(registry.mutex);
        registry.handles.erase(std::remove_if(registry.handles.begin(), registry.handles.end(),
                                              [handle](const OpenFileHandle& entry) { return entry.handle == handle; }),
                               registry.handles.end());
    } catch (...) {
        /**
         * @brief 用户数据析构不抛出异常；此处无法从互斥锁失败恢复，但也不得终止状态析构。
         */
    }
}

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

struct FileOpenError {
    Str filename;
    Str mode;
    int code = 0;
    Str message;
};

static FILE* rawFopen(const char* filename, const char* mode) {
#ifdef _MSC_VER
    return _fsopen(filename, mode, _SH_DENYNO);
#else
    return std::fopen(filename, mode);
#endif
}

static std::expected<FILE*, FileOpenError> tryFopen(StrView filename, StrView mode) {
    Str ownedFilename(filename);
    Str ownedMode(mode);

    errno = 0;
    FILE* fp = rawFopen(ownedFilename.c_str(), ownedMode.c_str());
    if (fp == nullptr) {
        const int errorCode = errno;
        return std::unexpected(FileOpenError{
            ownedFilename,
            ownedMode,
            errorCode,
            errnoMessage(errorCode),
        });
    }

    return fp;
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
static i32 pushResult(LuaState* L, bool success, const Value& successValue = Value(true),
                      const char* filename = nullptr) {
    if (success) {
        L->pushValue(successValue);
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

static FileHandleData* toFileHandle(LuaState* L, i32 idx) {
    // 检查是否为 userdata
    if (!L->isUserdata(idx)) {
        return nullptr;
    }

    Value val = L->at(idx);
    if (!val.isUserdata()) {
        return nullptr;
    }

    Userdata* ud = val.asUserdata();
    if (ud->getDataSize() != sizeof(FileHandleData)) {
        return nullptr;
    }

    return static_cast<FileHandleData*>(ud->getData());
}

static Function* createCClosureWithClosedUpvalues(LuaState* L, CFunction func, const Vec<Value>& upvalues) {
    Function* closure = L->getGlobalState().getGC().create<Function>(func);

    for (const Value& value : upvalues) {
        Upvalue* uv = L->getGlobalState().getGC().create<Upvalue>(value);
        closure->addUpvalue(uv);
    }

    return closure;
}

static Function* getCurrentClosure(LuaState* L) {
    const CallInfo& ci = L->getCurrentCallInfo();
    Value funcVal = L->getStack()[ci.func];
    if (!funcVal.isFunction()) {
        L->error("io iterator: current function is invalid");
    }
    return funcVal.asFunction();
}

static Value getClosureUpvalueValue(LuaState* L, usize index) {
    Function* closure = getCurrentClosure(L);
    Upvalue* uv = closure->getUpvalue(index);
    if (uv == nullptr) {
        L->error("io iterator: missing closure upvalue");
    }
    return uv->getValue(L->getStack());
}

static Value getDefaultInputHandleValue(LuaState* L) {
    Value val = L->getGlobal(IO_INPUT);
    if (toFileHandle(val) != nullptr) {
        return val;
    }

    Value ioTableVal = L->getGlobal("io");
    if (ioTableVal.isTable()) {
        auto& pool = L->getGlobalState().getStringPool();
        Value stdinVal = ioTableVal.asTable()->get(Value(pool.intern("stdin")));
        if (toFileHandle(stdinVal) != nullptr) {
            return stdinVal;
        }
    }

    Userdata* ud = createFileHandle(L, stdin, false, nullptr, false);
    return Value(ud);
}

static Value getDefaultOutputHandleValue(LuaState* L) {
    Value val = L->getGlobal(IO_OUTPUT);
    if (toFileHandle(val) != nullptr) {
        return val;
    }

    Value ioTableVal = L->getGlobal("io");
    if (ioTableVal.isTable()) {
        auto& pool = L->getGlobalState().getStringPool();
        Value stdoutVal = ioTableVal.asTable()->get(Value(pool.intern("stdout")));
        if (toFileHandle(stdoutVal) != nullptr) {
            return stdoutVal;
        }
    }

    Userdata* ud = createFileHandle(L, stdout, false, nullptr, false);
    return Value(ud);
}

static i32 pushLinesIterator(LuaState* L, const Value& fileHandle, bool autoClose, i32 firstFormatArg = 0) {
    Vec<Value> upvalues;
    upvalues.push_back(fileHandle);
    upvalues.push_back(Value(autoClose));
    for (i32 i = firstFormatArg; i > 0 && i <= L->getTop(); ++i) {
        upvalues.push_back(L->at(i));
    }

    Function* iter = createCClosureWithClosedUpvalues(L, lines_iterator, upvalues);
    L->pushFunction(iter);
    return 1;
}

static FileHandleData* toFileHandle(const Value& val) {
    if (!val.isUserdata()) {
        return nullptr;
    }

    Userdata* ud = val.asUserdata();
    if (ud == nullptr || ud->getDataSize() != sizeof(FileHandleData)) {
        return nullptr;
    }

    return static_cast<FileHandleData*>(ud->getData());
}

FileHandleData* checkFilePtr(LuaState* L, i32 idx) {
    FileHandleData* handle = toFileHandle(L, idx);
    if (!handle) {
        L->error("bad argument (FILE* expected)");
    }
    return handle;
}

static i32 closeFileHandle(FileHandleData* handle) {
    if (handle == nullptr) {
        return 0;
    }

    OpenFileRegistry& registry = openFileRegistry();
    std::lock_guard lock(registry.mutex);
    if (handle->get() == nullptr) {
        return 0;
    }
    registry.handles.erase(std::remove_if(registry.handles.begin(), registry.handles.end(),
                                          [handle](const OpenFileHandle& entry) { return entry.handle == handle; }),
                           registry.handles.end());
    return handle->close();
}

static bool handlePathMatches(FileHandleData* handle, const char* path) {
    return handle != nullptr && path != nullptr && !handle->path.empty() && handle->path == path;
}

bool releaseFileHandlesForPath(LuaState* L, const char* path) {
    if (L == nullptr || path == nullptr) {
        return false;
    }

    bool released = false;
    OpenFileRegistry& registry = openFileRegistry();
    std::lock_guard lock(registry.mutex);
    for (usize i = 0; i < registry.handles.size();) {
        const OpenFileHandle& entry = registry.handles[i];
        if (entry.owner != &L->getGlobalState()) {
            ++i;
            continue;
        }
        FileHandleData* handle = entry.handle;
        if (handlePathMatches(handle, path) && handle->get() != nullptr) {
            registry.handles.erase(registry.handles.begin() + static_cast<std::ptrdiff_t>(i));
            (void)handle->close();
            released = true;
            continue;
        }
        ++i;
    }
    return released;
}

Userdata* createFileHandle(LuaState* L, FILE* fp, bool isPipe, const char* path, bool ownsFile) {
    FileHandleData::FilePtr pendingFile(fp, FileCloser{isPipe, ownsFile});

    // 创建 userdata
    Userdata* ud = L->getGlobalState().getGC().create<Userdata>(sizeof(FileHandleData));
    Table* environment = L->getGlobalTable();
    if (L->getCurrentCI() != 0) {
        const CallInfo& ci = L->getCurrentCallInfo();
        if (ci.func < L->getStack().size() && L->getStack().at(ci.func).isFunction()) {
            Function* closure = L->getStack().at(ci.func).asFunction();
            if (closure != nullptr && closure->getEnv() != nullptr) {
                environment = closure->getEnv();
            }
        }
    }
    ud->setEnvironment(environment);

    // 设置文件句柄元数据
    FileHandleData* handle = ud->constructData<FileHandleData>();
    handle->owner = &L->getGlobalState();
    handle->reset(pendingFile.release(), isPipe, ownsFile);
    if (path != nullptr) {
        handle->path = path;
    }

    // 获取文件元表
    GCString* mtName = L->getGlobalState().getStringPool().intern(FILE_HANDLE_METATABLE);
    Value mtVal = L->getGlobal(mtName->getData());
    if (mtVal.isTable()) {
        ud->setMetatable(mtVal.asTable());
    }

    /**
     * @brief 所有可能分配或抛出的操作完成后才发布句柄。
     *
     * 当内存限制注入中止用户数据构造时，这可避免进程级句柄注册表出现悬空指针。
     */
    if (handle->get() != nullptr) {
        OpenFileRegistry& registry = openFileRegistry();
        std::lock_guard lock(registry.mutex);
        registry.handles.push_back(OpenFileHandle{&L->getGlobalState(), handle});
    }

    return ud;
}

FILE* getDefaultInput(LuaState* L) {
    FileHandleData* handle = toFileHandle(getDefaultInputHandleValue(L));
    return handle ? handle->get() : nullptr;
}

FILE* getDefaultOutput(LuaState* L) {
    FileHandleData* handle = toFileHandle(getDefaultOutputHandleValue(L));
    return handle ? handle->get() : nullptr;
}

void setDefaultInput(LuaState* L, FILE* fp) {
    Userdata* ud = createFileHandle(L, fp, false, nullptr, false);
    L->setGlobal(IO_INPUT, Value(ud));
}

void setDefaultOutput(LuaState* L, FILE* fp) {
    Userdata* ud = createFileHandle(L, fp, false, nullptr, false);
    L->setGlobal(IO_OUTPUT, Value(ud));
}

// =====================================================================
// 读取辅助函数
// =====================================================================

/**
 * @brief 读取一行（不包括换行符）
 */
static bool readLine(LuaState* L, FILE* fp) {
    LuaString line(LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
    i32 c;

    while ((c = std::fgetc(fp)) != EOF && c != '\n') {
        line += static_cast<char>(c);
    }

    if (line.empty() && c == EOF) {
        return false; // EOF
    }

    GCString* str = L->getGlobalState().getStringPool().intern(line.data(), line.size());
    L->pushString(str);
    return true;
}

/**
 * @brief 读取指定字符数
 */
static bool readChars(LuaState* L, FILE* fp, usize count) {
    if (count == 0) {
        i32 c = std::fgetc(fp);
        if (c == EOF) {
            return false;
        }
        std::ungetc(c, fp);
        GCString* str = L->getGlobalState().getStringPool().intern("", 0);
        L->pushString(str);
        return true;
    }

    LuaString buffer(LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
    buffer.reserve(count);

    for (usize i = 0; i < count; i++) {
        i32 c = std::fgetc(fp);
        if (c == EOF) {
            if (i == 0) {
                return false; // 起始位置即到达文件末尾
            }
            break;
        }
        buffer += static_cast<char>(c);
    }

    GCString* str = L->getGlobalState().getStringPool().intern(buffer.data(), buffer.size());
    L->pushString(str);
    return true;
}

/**
 * @brief 读取整个文件
 */
static bool readAll(LuaState* L, FILE* fp) {
    LuaString content(LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
    i32 c;

    while ((c = std::fgetc(fp)) != EOF) {
        content += static_cast<char>(c);
    }

    GCString* str = L->getGlobalState().getStringPool().intern(content.data(), content.size());
    L->pushString(str);
    return true;
}

/**
 * @brief 读取一个数字
 */
static bool readNumber(LuaState* L, FILE* fp) {
    LuaString token(LuaStdAllocator<char>(L->getGlobalState().getAllocator()));

    int c = std::fgetc(fp);
    while (c != EOF && std::isspace(static_cast<unsigned char>(c))) {
        c = std::fgetc(fp);
    }

    if (c == EOF) {
        return false;
    }

    while (c != EOF) {
        char ch = static_cast<char>(c);
        bool isNumericChar = std::isdigit(static_cast<unsigned char>(ch)) || ch == '+' || ch == '-' || ch == '.' ||
                             ch == 'e' || ch == 'E';

        if (!isNumericChar) {
            std::ungetc(c, fp);
            break;
        }

        token += ch;
        c = std::fgetc(fp);
    }

    if (token.empty()) {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    double v = std::strtod(token.c_str(), &end);
    if (end == token.c_str() || *end != '\0') {
        return false;
    }

    L->pushNumber(static_cast<f64>(v));
    return true;
}

// =====================================================================
// I/O库函数实现
// =====================================================================

i32 io_open(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
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
    auto opened = tryFopen(filename, mode);
    if (!opened) {
        return pushResult(L, false, Value(true), filename);
    }
    FILE* fp = *opened;

    // 创建文件句柄
    Userdata* ud = createFileHandle(L, fp, false, filename);
    L->pushUserdata(ud);

    return 1;
}

i32 io_close(LuaState* L) {
    FileHandleData* handle = nullptr;

    if (L->getTop() == 0) {
        handle = toFileHandle(getDefaultOutputHandleValue(L));
        if (!handle) {
            L->error("io.close: no default output file");
        }
    } else {
        handle = checkFilePtr(L, 1);
    }

    if (handle->get() == nullptr) {
        L->error("attempt to use a closed file");
    }

    i32 result = closeFileHandle(handle);
    return pushResult(L, result == 0);
}

// 前向声明
static i32 f_read_impl(LuaState* L, FILE* fp, i32 firstArg);
static i32 f_write_impl(LuaState* L, FileHandleData* handle, i32 firstArg, const Value& successValue);

i32 io_read(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    FILE* fp = getDefaultInput(L);
    if (!fp) {
        L->error("attempt to use a closed file");
    }
    return f_read_impl(L, fp, 1);
}

i32 io_write(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    Value outputHandle = getDefaultOutputHandleValue(L);
    FileHandleData* handle = toFileHandle(outputHandle);
    if (!handle || !handle->get()) {
        L->error("attempt to use a closed file");
    }
    return f_write_impl(L, handle, 1, outputHandle);
}

i32 io_flush(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    FILE* fp = getDefaultOutput(L);
    if (!fp) {
        L->error("attempt to use a closed file");
    }
    return pushResult(L, std::fflush(fp) == 0);
}

i32 io_input(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    if (L->getTop() == 0) {
        L->pushValue(getDefaultInputHandleValue(L));
        return 1;
    } else {
        if (L->isString(1)) {
            const char* filename = L->toString(1);
            auto opened = tryFopen(filename, "r");
            if (!opened) {
                fileError(L, 1, filename);
            }
            FILE* fp = *opened;
            Userdata* ud = createFileHandle(L, fp, false, filename);
            Value handleValue(ud);
            L->setGlobal(IO_INPUT, handleValue);
            L->pushValue(handleValue);
            return 1;
        } else {
            checkFilePtr(L, 1);
            L->setGlobal(IO_INPUT, L->at(1));
        }
        L->pushValue(1);
        return 1;
    }
}

i32 io_output(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    if (L->getTop() == 0) {
        L->pushValue(getDefaultOutputHandleValue(L));
        return 1;
    } else {
        if (L->isString(1)) {
            const char* filename = L->toString(1);
            auto opened = tryFopen(filename, "w");
            if (!opened) {
                fileError(L, 1, filename);
            }
            FILE* fp = *opened;
            Userdata* ud = createFileHandle(L, fp, false, filename);
            Value handleValue(ud);
            L->setGlobal(IO_OUTPUT, handleValue);
            L->pushValue(handleValue);
            return 1;
        } else {
            checkFilePtr(L, 1);
            L->setGlobal(IO_OUTPUT, L->at(1));
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

    FileHandleData* handle = toFileHandle(L, 1);
    if (!handle) {
        L->pushNil();
        return 1;
    }

    GCString* typeStr;
    if (handle->get()) {
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
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    Function* closure = getCurrentClosure(L);
    Value fileHandle = getClosureUpvalueValue(L, 0);
    Value autoCloseVal = getClosureUpvalueValue(L, 1);

    FileHandleData* handle = toFileHandle(fileHandle);
    if (!handle) {
        L->error("io.lines iterator: invalid file handle");
    }

    if (handle->get() == nullptr) {
        L->error("io.lines iterator: file is already closed");
    }

    usize formatCount = closure->getUpvalueCount() > 2 ? closure->getUpvalueCount() - 2 : 0;

    if (formatCount == 0) {
        if (readLine(L, handle->get())) {
            return 1;
        }

        if (autoCloseVal.isBoolean() && autoCloseVal.asBoolean()) {
            closeFileHandle(handle);
        }

        return 0;
    }

    L->setTop(0);
    for (usize i = 0; i < formatCount; ++i) {
        Upvalue* uv = closure->getUpvalue(i + 2);
        if (uv == nullptr) {
            L->error("io iterator: missing format upvalue");
        }
        L->pushValue(uv->getValue(L->getStack()));
    }

    i32 nresults = f_read_impl(L, handle->get(), 1);
    if (nresults <= 0 || (L->getTop() >= 1 && L->at(-nresults).isNil())) {
        if (autoCloseVal.isBoolean() && autoCloseVal.asBoolean()) {
            closeFileHandle(handle);
        }
        L->setTop(0);
        return 0;
    }

    return nresults;
}

i32 io_lines(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    if (L->getTop() == 0) {
        return pushLinesIterator(L, getDefaultInputHandleValue(L), false);
    }

    if (L->isString(1)) {
        const char* filename = L->toString(1);
        auto opened = tryFopen(filename, "r");
        if (!opened) {
            fileError(L, 1, filename);
        }
        FILE* fp = *opened;
        Userdata* ud = createFileHandle(L, fp, false, filename);
        return pushLinesIterator(L, Value(ud), true, 2);
    }

    L->error("io.lines: string expected");
}

i32 io_tmpfile(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    FILE* fp = safeTmpfile();
    if (!fp) {
        return pushResult(L, false);
    }

    Userdata* ud = createFileHandle(L, fp, false);
    L->pushUserdata(ud);
    return 1;
}

i32 io_popen(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Process);
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
    if (std::strcmp(command, "ls") == 0 && std::system("where ls >NUL 2>NUL") != 0) {
        L->error("io.popen: command not available");
    }
    FILE* fp = _popen(command, mode);
#else
    FILE* fp = popen(command, mode);
#endif

    if (!fp) {
        return pushResult(L, false, Value(true), command);
    }

    // 创建文件句柄
    Userdata* ud = createFileHandle(L, fp, true);
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
    i32 lastArg = L->getTop();
    i32 nargs = lastArg - firstArg + 1;
    i32 n = 0;
    bool success = true;

    if (nargs == 0) {
        // 默认读取一行
        success = readLine(L, fp);
        if (success) {
            n = 1;
        } else {
            L->pushNil();
            n = 1;
        }
    } else {
        // 处理每个参数
        for (i32 i = firstArg; i <= lastArg; i++) {
            if (L->isNumber(i)) {
                // 读取指定字符数
                f64 num = L->toNumber(i);
                if (num < 0) {
                    L->error("invalid format");
                }
                success = readChars(L, fp, static_cast<usize>(num));
            } else if (L->isString(i)) {
                // 读取格式
                const char* fmt = L->toString(i);
                if (std::strcmp(fmt, "*n") == 0 || std::strcmp(fmt, "*number") == 0) {
                    success = readNumber(L, fp);
                } else if (std::strcmp(fmt, "*a") == 0 || std::strcmp(fmt, "*all") == 0) {
                    success = readAll(L, fp);
                } else if (std::strcmp(fmt, "*l") == 0 || std::strcmp(fmt, "*line") == 0) {
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
static i32 f_write_impl(LuaState* L, FileHandleData* handle, i32 firstArg, const Value& successValue) {
    FILE* fp = handle->get();
    i32 nargs = L->getTop() - firstArg + 1;
    bool success = true;
    bool shouldFlushLine = false;

    for (i32 i = firstArg; i <= L->getTop(); i++) {
        if (L->isString(i)) {
            GCString* str = L->at(i).asString();
            const usize len = str->getLength();
            if (std::fwrite(str->c_str(), 1, len, fp) != len) {
                success = false;
                break;
            }
            shouldFlushLine = shouldFlushLine || std::memchr(str->c_str(), '\n', len) != nullptr;
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

    if (nargs <= 0) {
        success = true;
    }

    if (success && handle->lineBuffered && shouldFlushLine) {
        success = std::fflush(fp) == 0;
    }

    return pushResult(L, success, successValue);
}

i32 f_close(LuaState* L) {
    FileHandleData* handle = checkFilePtr(L, 1);

    if (!handle->get()) {
        L->error("attempt to use a closed file");
    }

    i32 result = closeFileHandle(handle);
    return pushResult(L, result == 0);
}

i32 f_read(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    FileHandleData* handle = checkFilePtr(L, 1);
    if (!handle->get()) {
        L->error("attempt to use a closed file");
    }
    return f_read_impl(L, handle->get(), 2);
}

i32 f_write(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    FileHandleData* handle = checkFilePtr(L, 1);
    if (!handle->get()) {
        L->error("attempt to use a closed file");
    }

    return f_write_impl(L, handle, 2, L->at(1));
}

i32 f_flush(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    FileHandleData* handle = checkFilePtr(L, 1);
    if (!handle->get()) {
        L->error("attempt to use a closed file");
    }
    return pushResult(L, std::fflush(handle->get()) == 0);
}

i32 f_seek(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    FileHandleData* handle = checkFilePtr(L, 1);
    if (!handle->get()) {
        L->error("attempt to use a closed file");
    }

    // 获取 whence 参数
    i32 whence = SEEK_CUR; // 默认
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
        offset = static_cast<long>(checkedIOInteger(L, 3, "seek offset has no valid integer representation"));
    }

    // 执行 seek
    if (std::fseek(handle->get(), offset, whence) != 0) {
        return pushResult(L, false, Value(true));
    }

    // 返回新位置
    long pos = std::ftell(handle->get());
    L->pushNumber(static_cast<f64>(pos));
    return 1;
}

i32 f_setvbuf(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    FileHandleData* handle = checkFilePtr(L, 1);
    if (!handle->get()) {
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
    }

    usize size = BUFSIZ;
    if (L->getTop() >= 3 && L->isNumber(3)) {
        const i32 requestedSize = checkedIOInteger(L, 3, "buffer size has no valid integer representation");
        if (requestedSize < 0) {
            L->error("buffer size must be non-negative");
        }
        size = static_cast<usize>(requestedSize);
    }

    i32 result = std::setvbuf(handle->get(), nullptr, m, size);
    if (result == 0) {
        handle->lineBuffered = (m == _IOLBF);
    }
    return pushResult(L, result == 0);
}

i32 f_lines(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    FileHandleData* handle = checkFilePtr(L, 1);
    if (!handle->get()) {
        L->error("attempt to use a closed file");
    }

    return pushLinesIterator(L, L->at(1), false, 2);
}

// =====================================================================
// 元表方法实现
// =====================================================================

i32 io_gc(LuaState* L) {
    FileHandleData* handle = toFileHandle(L, 1);
    if (!handle) {
        L->error(std::format("bad argument #1 to '__gc' (FILE* expected, got {})", L->typeName(L->type(1))).c_str());
    }
    if (handle && handle->get()) {
        closeFileHandle(handle);
    }
    return 0;
}

i32 io_tostring(LuaState* L) {
    FileHandleData* handle = toFileHandle(L, 1);
    if (!handle) {
        L->pushString(L->getGlobalState().getStringPool().intern("not a file"));
        return 1;
    }

    std::string str;
    if (handle->get()) {
        str = std::format("file ({})", static_cast<void*>(handle->get()));
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

    /**
     * @brief 注册内容反映已配置方案；操作级检查仍保护在后续限制前捕获的函数。
     */
    FunctionRegistrar registrar(L);
    registrar.addGlobal("close", io_close).addGlobal("type", io_type);

    const SandboxPolicy& policy = L->getGlobalState().getSandboxPolicy();
    if (policy.allows(SandboxCapability::Filesystem)) {
        registrar.addGlobal("open", io_open)
            .addGlobal("read", io_read)
            .addGlobal("write", io_write)
            .addGlobal("flush", io_flush)
            .addGlobal("input", io_input)
            .addGlobal("output", io_output)
            .addGlobal("lines", io_lines)
            .addGlobal("tmpfile", io_tmpfile);
    }
    if (policy.allows(SandboxCapability::Process)) {
        registrar.addGlobal("popen", io_popen);
    }
    registrar.commitToTable(ioTable);

    // 创建文件句柄元表
    Table* fileMT = L->getGlobalState().getGC().create<Table>();

    // 注册文件方法
    FunctionRegistrar fileRegistrar(L);
    fileRegistrar.addGlobal("close", f_close);
    if (policy.allows(SandboxCapability::Filesystem)) {
        fileRegistrar.addGlobal("read", f_read)
            .addGlobal("write", f_write)
            .addGlobal("flush", f_flush)
            .addGlobal("seek", f_seek)
            .addGlobal("setvbuf", f_setvbuf)
            .addGlobal("lines", f_lines);
    }
    fileRegistrar.commitToTable(fileMT);

    // 设置元方法
    GCString* gcKey = L->getGlobalState().getStringPool().intern("__gc");
    GCString* tostringKey = L->getGlobalState().getStringPool().intern("__tostring");
    GCString* indexKey = L->getGlobalState().getStringPool().intern("__index");

    FunctionRegistrar::registerToTable(L, fileMT, gcKey->c_str(), io_gc);
    FunctionRegistrar::registerToTable(L, fileMT, tostringKey->c_str(), io_tostring);

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
    Userdata* stdinHandle = createFileHandle(L, stdin, false, nullptr, false);
    Userdata* stdoutHandle = createFileHandle(L, stdout, false, nullptr, false);
    Userdata* stderrHandle = createFileHandle(L, stderr, false, nullptr, false);

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
    L->setGlobal(IO_INPUT, Value(stdinHandle));
    L->setGlobal(IO_OUTPUT, Value(stdoutHandle));
}

void openIOLib(LuaState* L) {
    if (!L) {
        return;
    }

    L->requireStandardLibrary("io");
    IOLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
