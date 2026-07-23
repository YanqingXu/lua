/**
 * @file packagelib.cpp
 * @brief Lua 包与模块库实现
 *
 * 实现 Lua 5.1.5 包系统：require()、module() 与 package.* 表字段。
 *
 * 设计说明：
 * - package.loaded 缓存所有成功加载的模块
 * - package.preload 允许 C 代码预先注册加载器函数
 * - package.loaders 是按顺序尝试的搜索器函数序列
 * - 默认加载器：[1] 预加载，[2] Lua 文件，[3] C 库，[4] 一体式 C 库
 * - require() 具有幂等性：重复调用返回缓存值
 * - module() 创建模块表并调整环境
 * @author Lua C++ 项目
 * @date 2026-04-10
 */

#include "lib/packagelib.hpp"
#include "lib/lib_registry.hpp"
#include "lib/lib_manager.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "runtime/native_module_registry.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/global_state.hpp"
#include "vm/vm.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include <array>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#include <limits.h>
#include <unistd.h>
#endif

namespace Lua {

// =====================================================================
// 内部键——用于从注册表或全局 package 表中获取包子表
// =====================================================================

static constexpr const char* PACKAGE_TABLE_NAME = "package";
static constexpr StrView PACKAGE_REGISTRY_KEY = "_PACKAGE_TABLE";

// 默认路径
#ifdef _WIN32
static constexpr StrView LUA_DEFAULT_PATH = ".\\?.lua;"
                                            ".\\?\\init.lua;"
                                            "!\\lua\\?.lua;"
                                            "!\\lua\\?\\init.lua";
static constexpr StrView LUA_DEFAULT_CPATH = ".\\?.dll;"
                                             "!\\?.dll;"
                                             "!\\loadall.dll";
static constexpr StrView LUA_PATH_SEP = ";";
static constexpr StrView LUA_PATH_MARK = "?";
static constexpr StrView LUA_DIR_SEP = "\\";
static constexpr StrView LUA_EXEC_DIR = "!";
static constexpr StrView LUA_IGMARK = "-";
static constexpr StrView LUA_OFSEP = "_";
#else
static constexpr StrView LUA_DEFAULT_PATH = "./?.lua;"
                                            "./?/init.lua;"
                                            "/usr/local/share/lua/5.1/?.lua;"
                                            "/usr/local/share/lua/5.1/?/init.lua";
static constexpr StrView LUA_DEFAULT_CPATH = "./?.so;"
                                             "/usr/local/lib/lua/5.1/?.so";
static constexpr StrView LUA_PATH_SEP = ";";
static constexpr StrView LUA_PATH_MARK = "?";
static constexpr StrView LUA_DIR_SEP = "/";
static constexpr StrView LUA_EXEC_DIR = "!";
static constexpr StrView LUA_IGMARK = "-";
static constexpr StrView LUA_OFSEP = "_";
#endif

// 配置字符串：分隔符、目录分隔符、替换标记、执行目录、忽略标记，以换行分隔
static constexpr StrView LUA_CONFIG_STRING =
#ifdef _WIN32
    "\\\n;\n?\n!\n-";
#else
    "/\n;\n?\n!\n-";
#endif

// =====================================================================
// 辅助函数：从全局环境获取 package 表
// =====================================================================

static Table* getPackageTable(LuaState* L) {
    Value pkgVal = L->getGlobal(PACKAGE_TABLE_NAME);
    if (pkgVal.isTable()) {
        return pkgVal.asTable();
    }

    GCString* registryKey = L->getGlobalState().getStringPool().intern(PACKAGE_REGISTRY_KEY);
    Value registryVal = L->getGlobalState().getRegistry()->get(Value(registryKey));
    if (registryVal.isTable()) {
        return registryVal.asTable();
    }
    return nullptr;
}

// =====================================================================
// 辅助函数：从 package 表获取子表
// =====================================================================

static Table* getPackageSubTable(LuaState* L, const char* fieldName) {
    Table* pkg = getPackageTable(L);
    if (!pkg)
        return nullptr;

    GCString* key = L->getGlobalState().getStringPool().intern(fieldName);
    Value val = pkg->get(Value(key));
    if (val.isTable()) {
        return val.asTable();
    }
    return nullptr;
}

static LuaString makePackageBuffer(LuaState* L) {
    return LuaString(LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
}

static void appendPackageBuffer(LuaState* L, LuaString& output, StrView text) {
    const ResourcePolicy& policy = L->getGlobalState().getResourcePolicy();
    const usize limit = (std::min)(policy.maxStringBytes, policy.maxOutputBytes);
    if (text.size() > limit || output.size() > limit - text.size()) {
        L->error("package: diagnostic exceeds resource limit");
    }
    L->consumeNativeWork(text.empty() ? 1 : static_cast<u64>(text.size()));
    output.append(text.data(), text.size());
}

// =====================================================================
// 辅助函数：从 package 表获取字符串字段
// =====================================================================

static Str getPackageStringField(LuaState* L, const char* fieldName) {
    Table* pkg = getPackageTable(L);
    if (!pkg)
        return "";

    GCString* key = L->getGlobalState().getStringPool().intern(fieldName);
    Value val = pkg->get(Value(key));
    if (val.isString()) {
        return val.asString()->c_str();
    }
    return "";
}

// =====================================================================
// 路径搜索：在每个路径模板中用模块名替换问号
// =====================================================================

/** @brief 将 `str` 中出现的所有 `pattern` 替换为 `replacement`。 */
static Str replaceAll(StrView str, StrView pattern, StrView replacement) {
    Str result;
    result.reserve(str.size());
    usize pos = 0;
    usize patLen = pattern.size();
    while (pos < str.size()) {
        usize found = str.find(pattern, pos);
        if (found == Str::npos) {
            result.append(str.substr(pos));
            break;
        }
        result.append(str.substr(pos, found - pos));
        result.append(replacement);
        pos = found + patLen;
    }
    return result;
}

/** @brief 将模块名中的点转换为目录分隔符，以便搜索路径。 */
static Str moduleNameToPath(const Str& modname) {
    return replaceAll(modname, ".", LUA_DIR_SEP);
}

static Str executablePath() {
#ifdef _WIN32
    std::array<char, MAX_PATH> buffer{};
    DWORD len = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (len == 0 || len >= buffer.size()) {
        return "";
    }
    return Str(buffer.data(), len);
#elif defined(__APPLE__)
    uint32_t capacity = PATH_MAX;
    Vec<char> buffer(static_cast<usize>(capacity) + 1, '\0');
    if (_NSGetExecutablePath(buffer.data(), &capacity) != 0) {
        buffer.assign(static_cast<usize>(capacity) + 1, '\0');
        if (_NSGetExecutablePath(buffer.data(), &capacity) != 0) {
            return "";
        }
    }
    return Str(buffer.data());
#else
    std::array<char, PATH_MAX> buffer{};
    ssize_t len = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (len <= 0) {
        return "";
    }
    return Str(buffer.data(), static_cast<usize>(len));
#endif
}

static Str executableDirectory() {
    Str path = executablePath();
    if (path.empty()) {
        return ".";
    }

    usize pos = path.find_last_of("/\\");
    if (pos == Str::npos) {
        return ".";
    }
    return path.substr(0, pos);
}

static Str applyExecutableDirectory(StrView pathTemplate) {
    return replaceAll(pathTemplate, LUA_EXEC_DIR, executableDirectory());
}

static Table* createPackageTableObject(LuaState* L) {
    return L->getGlobalState().getGC().create<Table>();
}

[[noreturn]] static void moduleNameConflict(LuaState* L, StrView modname) {
    LuaString msg = makePackageBuffer(L);
    appendPackageBuffer(L, msg, "name conflict for module '");
    appendPackageBuffer(L, msg, modname);
    appendPackageBuffer(L, msg, "'");
    L->error(msg.c_str());
}

static StrView modulePackagePrefix(StrView modname) {
    usize pos = modname.rfind('.');
    return pos == StrView::npos ? StrView() : modname.substr(0, pos + 1);
}

static Table* ensureChildTable(LuaState* L, Table* parent, StrView field, StrView modname) {
    auto& pool = L->getGlobalState().getStringPool();
    GCString* key = pool.intern(field);
    Value existing = parent->get(Value(key));
    if (existing.isTable()) {
        return existing.asTable();
    }
    if (!existing.isNil()) {
        moduleNameConflict(L, modname);
    }

    Table* child = createPackageTableObject(L);
    parent->set(Value(key), Value(child));
    return child;
}

static Table* findOrCreateGlobalModuleTable(LuaState* L, StrView modname) {
    auto& pool = L->getGlobalState().getStringPool();
    Table* parent = L->getGlobalTable();

    usize start = 0;
    while (start <= modname.size()) {
        usize dot = modname.find('.', start);
        bool isLast = dot == StrView::npos;
        StrView field = isLast ? modname.substr(start) : modname.substr(start, dot - start);
        if (field.empty()) {
            moduleNameConflict(L, modname);
        }

        if (isLast) {
            GCString* key = pool.intern(field);
            Value existing = parent->get(Value(key));
            if (existing.isTable()) {
                return existing.asTable();
            }
            if (!existing.isNil()) {
                moduleNameConflict(L, modname);
            }

            Table* modTable = createPackageTableObject(L);
            parent->set(Value(key), Value(modTable));
            return modTable;
        }

        parent = ensureChildTable(L, parent, field, modname);
        start = dot + 1;
    }

    moduleNameConflict(L, modname);
}

static void setGlobalModulePath(LuaState* L, StrView modname, Table* modTable) {
    auto& pool = L->getGlobalState().getStringPool();
    Table* parent = L->getGlobalTable();

    usize start = 0;
    while (start <= modname.size()) {
        usize dot = modname.find('.', start);
        bool isLast = dot == StrView::npos;
        StrView field = isLast ? modname.substr(start) : modname.substr(start, dot - start);
        if (field.empty()) {
            moduleNameConflict(L, modname);
        }

        GCString* key = pool.intern(field);
        if (isLast) {
            parent->set(Value(key), Value(modTable));
            return;
        }

        Value existing = parent->get(Value(key));
        if (!existing.isNil() && !existing.isTable()) {
            moduleNameConflict(L, modname);
        }
        parent = existing.isTable() ? existing.asTable() : ensureChildTable(L, parent, field, modname);
        start = dot + 1;
    }

    moduleNameConflict(L, modname);
}

static void setCallingLuaFunctionEnv(LuaState* L, Table* env) {
    if (L->getCurrentCI() == 0) {
        L->error("module: no calling Lua function");
    }

    LuaVector<CallInfo>& frames = L->getCallStack();
    Stack& stack = L->getStack();

    for (usize i = L->getCurrentCI(); i > 0; --i) {
        const CallInfo& caller = frames[i - 1];
        if (caller.func >= stack.size()) {
            continue;
        }

        Value& funcVal = stack.at(caller.func);
        if (!funcVal.isFunction()) {
            continue;
        }

        Function* func = funcVal.asFunction();
        if (func->isLuaFunction()) {
            func->setEnv(env);
            return;
        }
    }

    L->error("module: no calling Lua function");
}

static void callModuleOption(LuaState* L, const Value& option, Table* modTable) {
    if (!option.isFunction()) {
        L->error("bad argument to 'module' option (function expected)");
    }

    usize savedTop = L->getAbsoluteTop();
    try {
        RuntimeServices services(L->getGlobalState());
        L->pushValue(option);
        L->pushValue(Value(modTable));
        VM::call(services, L, 1, 0);
        L->getStack().setTop(savedTop);
        L->setAbsoluteTop(savedTop);
    } catch (...) {
        L->getStack().setTop(savedTop);
        L->setAbsoluteTop(savedTop);
        throw;
    }
}

/**
 * @brief 按给定路径模板字符串搜索文件
 * @return 找到的文件路径，未找到时返回空字符串
 * @note 失败时将尝试过的路径追加到 `errorBuf`，用于生成错误消息。
 */
static Str searchPath(const Str& name, const Str& pathStr, Str& errorBuf) {
    Str modPath = moduleNameToPath(name);

    // 按配置的路径分隔符拆分 pathStr
    usize pos = 0;
    while (pos <= pathStr.size()) {
        usize sep = pathStr.find(LUA_PATH_SEP, pos);
        if (sep == Str::npos)
            sep = pathStr.size();

        Str tmpl = pathStr.substr(pos, sep - pos);
        pos = sep + 1;

        if (tmpl.empty())
            continue;

        tmpl = applyExecutableDirectory(tmpl);

        // 用模块名替换问号，其中点已替换为目录分隔符
        Str filePath = replaceAll(tmpl, LUA_PATH_MARK, modPath);

        // 尝试打开文件
        std::ifstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            file.close();
            return filePath;
        }

        // 累积错误消息
        errorBuf += "\n\tno file '";
        errorBuf += filePath;
        errorBuf += "'";
    }
    return "";
}

// =====================================================================
// 辅助函数：加载 Lua 文件并返回已编译函数；失败时返回 nullptr，并将 nil 与错误压栈
// =====================================================================

static Function* loadLuaFile(LuaState* L, const Str& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return nullptr;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    Str source;
    source.resize(static_cast<usize>(size));
    if (!file.read(&source[0], size)) {
        return nullptr;
    }

    try {
        RuntimeServices services(L->getGlobalState());
        Parser parser(source, services);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);

        CodeGenerator codegen(services);
        Str chunkName = "@" + filename;
        Proto* proto = codegen.generate(chunk, chunkName);
        if (!proto) {
            return nullptr;
        }

        Function* func = L->getGlobalState().getGC().create<Function>(proto);
        func->setEnv(L->getGlobalTable());
        return func;

    } catch (...) {
        return nullptr;
    }
}

// =====================================================================
// 动态 C 库支持
// =====================================================================

enum class DynamicLookupStatus { Success, OpenFailure, InitFailure };

struct DynamicLookupResult {
    DynamicLookupStatus status;
    Function* function;
    Str message;
    bool linkedOnly;
};

static Function* createDynamicCFunction(LuaState* L, void* symbol) {
    ApiCFunction cfunc = reinterpret_cast<ApiCFunction>(symbol);
    Function* func = L->getGlobalState().getGC().create<Function>(cfunc);
    return func;
}

static DynamicLookupResult lookForDynamicFunction(LuaState* L, const Str& filename, const Str& functionName) {
    NativeModuleRegistry& modules = L->getGlobalState().getNativeModules();
    auto handle = modules.load(filename);
    if (!handle) {
        return {DynamicLookupStatus::OpenFailure, nullptr, handle.error(), false};
    }

    if (functionName == "*") {
        return {DynamicLookupStatus::Success, nullptr, Str(), true};
    }

    auto symbol = modules.findSymbol(*handle, functionName);
    if (!symbol) {
        return {DynamicLookupStatus::InitFailure, nullptr, symbol.error(), false};
    }

    return {DynamicLookupStatus::Success, createDynamicCFunction(L, *symbol), Str(), false};
}

static Str moduleNameToOpenFunction(const Str& modname) {
    Str name = modname;
    usize mark = name.find(LUA_IGMARK);
    if (mark != Str::npos) {
        name = name.substr(mark + LUA_IGMARK.size());
    }

    return "luaopen_" + replaceAll(name, ".", LUA_OFSEP);
}

[[noreturn]] static void dynamicLoadError(LuaState* L, const Str& modname, const Str& filename, const Str& message) {
    Str error = "error loading module '" + modname + "' from file '" + filename + "':\n\t" + message;
    L->error(error.c_str());
}

static void pushSearchError(LuaState* L, const Str& message) {
    L->setTop(0);
    L->pushString(L->getGlobalState().getStringPool().intern(message.c_str()));
}

// =====================================================================
// package.loaders[1]——预加载搜索器
// =====================================================================

i32 loader_preload(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1 || !L->at(1).isString()) {
        L->setTop(0);
        L->pushString(L->getGlobalState().getStringPool().intern("\n\tno field package.preload"));
        return 1;
    }

    GCString* modKey = L->at(1).asString();
    const StrView modname = modKey->view();
    auto& pool = L->getGlobalState().getStringPool();

    // 查询 package.preload[modname]
    Table* preload = getPackageSubTable(L, "preload");
    if (!preload) {
        L->setTop(0);
        L->pushString(pool.intern("\n\tno field package.preload"));
        return 1;
    }

    Value loaderVal = preload->get(Value(modKey));

    if (loaderVal.isFunction()) {
        L->setTop(0);
        L->pushValue(loaderVal);
        return 1;
    }

    // 未找到——返回错误字符串，但不抛出硬错误
    LuaString msg = makePackageBuffer(L);
    appendPackageBuffer(L, msg, "\n\tno field package.preload['");
    appendPackageBuffer(L, msg, modname);
    appendPackageBuffer(L, msg, "']");
    L->setTop(0);
    L->pushString(pool.intern(msg.data(), msg.size()));
    return 1;
}

// =====================================================================
// package.loaders[2]——Lua 文件搜索器
// =====================================================================

i32 loader_lua(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::Filesystem);
    i32 nargs = L->getTop();
    if (nargs < 1 || !L->at(1).isString()) {
        L->setTop(0);
        L->pushString(L->getGlobalState().getStringPool().intern("\n\tno field package.path"));
        return 1;
    }

    Str modname = L->at(1).asString()->c_str();
    auto& pool = L->getGlobalState().getStringPool();

    Str pathStr = getPackageStringField(L, "path");
    if (pathStr.empty()) {
        L->setTop(0);
        L->pushString(pool.intern("\n\tno field package.path"));
        return 1;
    }

    Str errorBuf;
    Str filename = searchPath(modname, pathStr, errorBuf);

    if (filename.empty()) {
        // 未找到——返回错误字符串
        L->setTop(0);
        L->pushString(pool.intern(errorBuf.c_str()));
        return 1;
    }

    // 已找到——编译并返回加载器函数
    Function* func = loadLuaFile(L, filename);
    if (!func) {
        Str msg = "\n\terror loading module '" + modname + "' from file '" + filename + "'";
        L->setTop(0);
        L->pushString(pool.intern(msg.c_str()));
        return 1;
    }

    L->setTop(0);
    L->pushValue(Value(func));
    return 1;
}

// =====================================================================
// package.loaders[3]——C 库搜索器
// =====================================================================

i32 loader_clib(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::NativeModules);
    i32 nargs = L->getTop();
    if (nargs < 1 || !L->at(1).isString()) {
        pushSearchError(L, "\n\tno C module name");
        return 1;
    }

    Str modname = L->at(1).asString()->c_str();

    Str pathStr = getPackageStringField(L, "cpath");
    if (pathStr.empty()) {
        pushSearchError(L, "\n\tno field package.cpath");
        return 1;
    }

    Str errorBuf;
    Str filename = searchPath(modname, pathStr, errorBuf);
    if (filename.empty()) {
        pushSearchError(L, errorBuf);
        return 1;
    }

    Str functionName = moduleNameToOpenFunction(modname);
    DynamicLookupResult lookup = lookForDynamicFunction(L, filename, functionName);
    if (lookup.status == DynamicLookupStatus::Success && lookup.function) {
        L->setTop(0);
        L->pushValue(Value(lookup.function));
        return 1;
    }

    dynamicLoadError(L, modname, filename, lookup.message);
}

// =====================================================================
// package.loaders[4]——一体式 C 库搜索器
// =====================================================================

i32 loader_clib_allinone(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::NativeModules);
    i32 nargs = L->getTop();
    if (nargs < 1 || !L->at(1).isString()) {
        L->setTop(0);
        return 0;
    }

    Str modname = L->at(1).asString()->c_str();
    usize dot = modname.find('.');
    if (dot == Str::npos || dot == 0) {
        L->setTop(0);
        return 0;
    }

    Str rootname = modname.substr(0, dot);
    Str pathStr = getPackageStringField(L, "cpath");
    if (pathStr.empty()) {
        pushSearchError(L, "\n\tno field package.cpath");
        return 1;
    }

    Str errorBuf;
    Str filename = searchPath(rootname, pathStr, errorBuf);
    if (filename.empty()) {
        pushSearchError(L, errorBuf);
        return 1;
    }

    Str functionName = moduleNameToOpenFunction(modname);
    DynamicLookupResult lookup = lookForDynamicFunction(L, filename, functionName);
    if (lookup.status == DynamicLookupStatus::Success && lookup.function) {
        L->setTop(0);
        L->pushValue(Value(lookup.function));
        return 1;
    }

    if (lookup.status == DynamicLookupStatus::InitFailure) {
        Str msg = "\n\tno module '" + modname + "' in file '" + filename + "'";
        pushSearchError(L, msg);
        return 1;
    }

    dynamicLoadError(L, modname, filename, lookup.message);
    return 1;
}

// =====================================================================
// require(modname)——加载并返回模块
// =====================================================================

i32 luaP_require(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("bad argument #1 to 'require' (string expected, got no value)");
    }
    if (!L->at(1).isString()) {
        L->error("bad argument #1 to 'require' (string expected)");
    }

    GCString* modKey = L->at(1).asString();
    const StrView modname = modKey->view();

    // 1. 检查 package.loaded[modname]
    Table* loaded = getPackageSubTable(L, "loaded");
    if (!loaded) {
        L->error("'package.loaded' table is missing");
    }

    Value cachedVal = loaded->get(Value(modKey));

    if (cachedVal.isTrue()) {
        // 已加载——返回缓存值
        L->setTop(0);
        L->pushValue(cachedVal);
        return 1;
    }

    // 2. 逐一尝试 package.loaders 中的加载器
    Table* loaders = getPackageSubTable(L, "loaders");
    if (!loaders) {
        L->error("'package.loaders' table is missing");
    }

    LuaString errorAccum = makePackageBuffer(L);
    appendPackageBuffer(L, errorAccum, "module '");
    appendPackageBuffer(L, errorAccum, modname);
    appendPackageBuffer(L, errorAccum, "' not found:");

    // 依次遍历 loaders[1]、loaders[2] 等
    for (i32 i = 1;; i++) {
        L->consumeNativeWork();
        Value loaderEntry = loaders->get(Value(static_cast<f64>(i)));
        if (loaderEntry.isNil()) {
            break; // 已无更多加载器
        }

        if (!loaderEntry.isFunction()) {
            continue; // 跳过非函数条目
        }

        Function* searcherFunc = loaderEntry.asFunction();

        // 调用搜索器：result = searcher(modname)
        // 使用 VM::call 正确建立调用帧，使 at(1) = modname
        L->setTop(0);
        L->pushValue(Value(searcherFunc));
        L->pushString(modKey);
        RuntimeServices services(L->getGlobalState());
        VM::call(services, L, 1, 1); // 1 个参数 modname，1 个结果

        // VM::call 返回后，结果位于栈上
        if (L->getTop() >= 1) {
            Value result = L->at(1);

            if (result.isFunction()) {
                // 找到加载器函数——以 modname 为参数调用
                Function* loaderFunc = result.asFunction();

                L->setTop(0);
                L->pushValue(Value(loaderFunc));
                L->pushString(modKey);

                Value moduleResult;

                try {
                    VM::call(services, L, 1, 1); // 1 个参数 modname，1 个结果

                    if (L->getTop() >= 1) {
                        moduleResult = L->at(1);
                    }
                } catch (const std::exception& e) {
                    LuaString msg = makePackageBuffer(L);
                    appendPackageBuffer(L, msg, "error loading module '");
                    appendPackageBuffer(L, msg, modname);
                    appendPackageBuffer(L, msg, "': ");
                    appendPackageBuffer(L, msg, e.what());
                    L->error(msg.c_str());
                }

                // 若加载器返回非 nil 值，则保存它
                if (!moduleResult.isNil()) {
                    loaded->set(Value(modKey), moduleResult);
                } else {
                    // 若无显式返回值，检查模块本身是否设置 package.loaded[modname]；否则设为 true
                    Value check = loaded->get(Value(modKey));
                    if (check.isNil()) {
                        moduleResult = Value(true);
                        loaded->set(Value(modKey), moduleResult);
                    } else {
                        moduleResult = check;
                    }
                }

                L->setTop(0);
                L->pushValue(moduleResult);
                return 1;

            } else if (result.isString()) {
                // 搜索器返回错误字符串
                appendPackageBuffer(L, errorAccum, result.asString()->view());
            }
            // 其他情况：类型不符合预期，跳过
        }
    }

    // 没有加载器成功
    L->error(errorAccum.c_str());
    return 0; // 不可达
}

// =====================================================================
// module(name [, ...])——创建模块
// =====================================================================

i32 luaP_module(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("bad argument #1 to 'module' (string expected, got no value)");
    }
    if (!L->at(1).isString()) {
        L->error("bad argument #1 to 'module' (string expected)");
    }

    GCString* modKey = L->at(1).asString();
    const StrView modname = modKey->view();
    auto& pool = L->getGlobalState().getStringPool();
    const usize optionCount = nargs > 1 ? static_cast<usize>(nargs - 1) : 0;
    L->consumeNativeWork(optionCount == 0 ? 1 : static_cast<u64>(optionCount));
    LuaVector<Value> options(LuaStdAllocator<Value>(L->getGlobalState().getAllocator()));
    options.reserve(optionCount);
    for (i32 i = 2; i <= nargs; ++i) {
        options.push_back(L->at(i));
    }

    // 1. 检查 package.loaded[modname]；不存在时创建或复用全局模块路径
    Table* loaded = getPackageSubTable(L, "loaded");
    if (!loaded) {
        L->error("'package.loaded' table is missing");
    }

    Value existingVal = loaded->get(Value(modKey));

    Table* modTable = nullptr;
    if (existingVal.isTable()) {
        modTable = existingVal.asTable();
        setGlobalModulePath(L, modname, modTable);
    } else {
        modTable = findOrCreateGlobalModuleTable(L, modname);

        // 将其存入 package.loaded
        loaded->set(Value(modKey), Value(modTable));
    }

    // 2. 设置 _NAME、_M 与 _PACKAGE 字段
    GCString* nameKey = pool.intern("_NAME");
    modTable->set(Value(nameKey), Value(modKey));

    GCString* mKey = pool.intern("_M");
    modTable->set(Value(mKey), Value(modTable));

    StrView packagePrefix = modulePackagePrefix(modname);
    GCString* packageKey = pool.intern("_PACKAGE");
    GCString* packageVal = pool.intern(packagePrefix);
    modTable->set(Value(packageKey), Value(packageVal));

    // 3. 将调用方 Lua 函数切换到模块环境
    setCallingLuaFunctionEnv(L, modTable);

    // 4. 应用选项函数，例如 package.seeall
    for (const Value& optVal : options) {
        callModuleOption(L, optVal, modTable);
    }

    L->setTop(0);
    return 0;
}

// =====================================================================
// package.loadlib(libname, funcname)——加载动态 C 库
// =====================================================================

i32 luaP_loadlib(LuaState* L) {
    L->requireSandboxCapability(SandboxCapability::NativeModules);
    auto& pool = L->getGlobalState().getStringPool();

    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("bad argument #1 to 'package.loadlib' (string expected, got no value)");
    }
    if (!L->at(1).isString()) {
        L->error("bad argument #1 to 'package.loadlib' (string expected)");
    }
    if (nargs < 2) {
        L->error("bad argument #2 to 'package.loadlib' (string expected, got no value)");
    }
    if (!L->at(2).isString()) {
        L->error("bad argument #2 to 'package.loadlib' (string expected)");
    }

    Str filename = L->at(1).asString()->c_str();
    Str functionName = L->at(2).asString()->c_str();

    DynamicLookupResult lookup = lookForDynamicFunction(L, filename, functionName);
    if (lookup.status == DynamicLookupStatus::Success) {
        L->setTop(0);
        if (lookup.linkedOnly) {
            L->pushBoolean(true);
        } else {
            L->pushValue(Value(lookup.function));
        }
        return 1;
    }

    L->setTop(0);
    L->pushNil();
    L->pushString(pool.intern(lookup.message.c_str()));
    L->pushString(pool.intern(lookup.status == DynamicLookupStatus::OpenFailure ? "open" : "init"));
    return 3;
}

// =====================================================================
// package.seeall(module)——设置模块环境以访问全部全局变量
// =====================================================================

i32 luaP_seeall(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("bad argument #1 to 'package.seeall' (table expected)");
    }

    if (!L->at(1).isTable()) {
        L->error("bad argument #1 to 'package.seeall' (table expected)");
    }

    Table* modTable = L->at(1).asTable();
    auto& pool = L->getGlobalState().getStringPool();

    // 将模块元表的 __index 设为 _G；若模块表没有元表，则创建一个
    Table* mt = modTable->getMetatable();
    if (!mt) {
        mt = L->getGlobalState().getGC().create<Table>();
        modTable->setMetatable(mt);
    }

    GCString* indexKey = pool.intern("__index");
    mt->set(Value(indexKey), Value(L->getGlobalTable()));

    return 0;
}

// =====================================================================
// package.searchpath(name, path [, sep [, rep]])
// 并非 Lua 5.1 正式接口，但属于实用辅助函数——暂不实现
// =====================================================================

// =====================================================================
// 库注册
// =====================================================================

void PackageLibModule::registerFunctions(LuaState* L) {
    if (!L) {
        return;
    }

    auto& pool = L->getGlobalState().getStringPool();
    auto& gc = L->getGlobalState().getGC();

    // ---- 创建 package 表 ----
    Table* pkgTable = FunctionRegistrar::createLibTable(L, PACKAGE_TABLE_NAME);
    if (!pkgTable) {
        L->error("Failed to create package library table");
        return;
    }

    GCString* registryKey = pool.intern(PACKAGE_REGISTRY_KEY);
    L->getGlobalState().getRegistry()->set(Value(registryKey), Value(pkgTable));

    // ---- 向 package 表注册函数 ----
    FunctionRegistrar packageRegistrar(L);
    packageRegistrar.addGlobal("seeall", luaP_seeall);
    const SandboxPolicy& policy = L->getGlobalState().getSandboxPolicy();
    if (policy.allows(SandboxCapability::NativeModules)) {
        packageRegistrar.addGlobal("loadlib", luaP_loadlib);
    }
    packageRegistrar.commitToTable(pkgTable);

    // ---- 注册全局函数：require、module ----
    FunctionRegistrar(L).addGlobal("require", luaP_require).addGlobal("module", luaP_module).commit();

    // ---- package.loaded 已加载模块表 ----
    Table* loadedTable = gc.create<Table>();
    GCString* loadedKey = pool.intern("loaded");
    pkgTable->set(Value(loadedKey), Value(loadedTable));

    /**
     * @brief 使用已打开的标准库预填充 package.loaded。
     *
     * 基础库保存为 _G，具名库保存其库表。require() 调用时会检测这些条目；当前先将 loaded
     * 保持为空，模块在首次 require() 时注册。
     */

    // ---- package.preload 预加载表 ----
    Table* preloadTable = gc.create<Table>();
    GCString* preloadKey = pool.intern("preload");
    pkgTable->set(Value(preloadKey), Value(preloadTable));

    // ---- package.path Lua 模块路径 ----
    GCString* pathKey = pool.intern("path");
    Str defaultPath = policy.allows(SandboxCapability::Filesystem) ? applyExecutableDirectory(LUA_DEFAULT_PATH) : Str();
    GCString* pathVal = pool.intern(defaultPath.c_str());
    pkgTable->set(Value(pathKey), Value(pathVal));

    // ---- package.cpath C 模块路径 ----
    GCString* cpathKey = pool.intern("cpath");
    Str defaultCPath =
        policy.allows(SandboxCapability::NativeModules) ? applyExecutableDirectory(LUA_DEFAULT_CPATH) : Str();
    GCString* cpathVal = pool.intern(defaultCPath.c_str());
    pkgTable->set(Value(cpathKey), Value(cpathVal));

    // ---- package.config 路径配置 ----
    GCString* configKey = pool.intern("config");
    GCString* configVal = pool.intern(LUA_CONFIG_STRING);
    pkgTable->set(Value(configKey), Value(configVal));

    // ---- package.loaders 加载器表 ----
    Table* loadersTable = gc.create<Table>();
    GCString* loadersKey = pool.intern("loaders");
    pkgTable->set(Value(loadersKey), Value(loadersTable));

    // loader[1] = 预加载搜索器
    Function* preloadSearcher = gc.create<Function>(loader_preload);
    loadersTable->set(Value(1.0), Value(preloadSearcher));

    i32 nextLoader = 2;
    if (policy.allows(SandboxCapability::Filesystem)) {
        Function* luaSearcher = gc.create<Function>(loader_lua);
        loadersTable->set(Value(static_cast<f64>(nextLoader++)), Value(luaSearcher));
    }
    if (policy.allows(SandboxCapability::NativeModules)) {
        Function* clibSearcher = gc.create<Function>(loader_clib);
        loadersTable->set(Value(static_cast<f64>(nextLoader++)), Value(clibSearcher));

        Function* allInOneSearcher = gc.create<Function>(loader_clib_allinone);
        loadersTable->set(Value(static_cast<f64>(nextLoader)), Value(allInOneSearcher));
    }
}

void PackageLibModule::initialize(LuaState* L) {
    if (!L) {
        return;
    }

    // 使用已打开的标准库预填充 package.loaded，使 require("math") 等调用可用
    Table* loaded = getPackageSubTable(L, "loaded");
    if (!loaded)
        return;

    auto& pool = L->getGlobalState().getStringPool();

    // 将库名称映射到对应的全局表条目
    static constexpr std::array<StrView, 9> stdlibs = {"_G",    "math",      "io",    "os",     "string",
                                                       "table", "coroutine", "debug", "package"};

    for (StrView name : stdlibs) {
        Value libVal = L->getGlobal(Str(name));
        if (!libVal.isNil()) {
            GCString* key = pool.intern(name);
            loaded->set(Value(key), libVal);
        }
    }
}

void openPackageLib(LuaState* L) {
    if (!L) {
        return;
    }

    L->requireStandardLibrary("package");
    PackageLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
