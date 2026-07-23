/**
 * @file iolib.hpp
 * @brief Lua I/O库：文件操作和标准输入输出功能
 *
 * 详细说明：
 * 本模块实现了Lua 5.1.5标准I/O库的完整功能，提供了文件打开、读写、关闭等操作，
 * 以及标准输入输出的处理。实现符合Lua标准规范，并遵循项目的现代C++编码风格。
 *
 * 主要功能：
 * 1. 文件操作：open, close, read, write, flush, seek, lines
 * 2. 标准I/O：input, output, stdin, stdout, stderr
 * 3. 文件类型检查：type
 * 4. 临时文件：tmpfile
 *
 * 文件打开模式：
 * - "r": 只读模式（默认）
 * - "w": 写入模式，清空现有内容
 * - "a": 追加模式
 * - "r+", "w+", "a+": 读写模式
 * - 添加 "b" 表示二进制模式
 *
 * 文件句柄类型：
 * 文件句柄通过 Userdata 实现，包含文件指针和元表。
 * 支持面向对象的方法调用（如 file:read()）。
 * @author Lua C++ 项目
 * @date 2025-12-19
 */

#pragma once

#include "common/types.hpp"
#include "lib/lib_module.hpp"
#include "vm/state/lua_state.hpp"
#include <cstdio>

namespace Lua {

// 前向声明
class Userdata;
class Table;
struct FileHandleData;

/**
 * @brief I/O库模块
 *
 * 实现Lua标准I/O库的所有功能。文件操作函数注册在全局表"io"中，
 * 同时文件句柄支持面向对象的方法调用。
 */
class IOLibModule : public LibModule {
public:
    const char* getName() const override {
        return "io";
    }

    void registerFunctions(LuaState* L) override;

    void initialize(LuaState* L) override;
};

/**
 * @brief 注册I/O库到全局环境
 * @param L Lua状态机指针
 *
 * 创建全局"io"表并注册所有I/O函数，同时设置文件元表。
 */
void openIOLib(LuaState* L);

// =====================================================================
// I/O库函数声明
// =====================================================================

/**
 * @brief io.open(filename [, mode]) - 打开文件
 *
 * 打开指定文件并返回文件句柄。
 *
 * @param L Lua状态机指针
 * @return 返回值数量（1个：文件句柄或nil+错误信息+错误码）
 */
i32 io_open(LuaState* L);

/**
 * @brief io.close([file]) - 关闭文件
 *
 * 关闭指定文件。如果不提供参数，关闭默认输出文件。
 *
 * @param L Lua状态机指针
 * @return 返回值数量（1个或3个）
 */
i32 io_close(LuaState* L);

/**
 * @brief io.read(...) - 从默认输入读取
 *
 * 从当前输入文件读取数据，支持多种格式。
 *
 * @param L Lua状态机指针
 * @return 返回值数量（读取的值的数量）
 */
i32 io_read(LuaState* L);

/**
 * @brief io.write(...) - 写入默认输出
 *
 * 向当前输出文件写入数据。
 *
 * @param L Lua状态机指针
 * @return 返回值数量（1个：输出文件句柄或nil+错误）
 */
i32 io_write(LuaState* L);

/**
 * @brief io.flush() - 刷新默认输出
 *
 * 刷新当前输出文件的缓冲区。
 *
 * @param L Lua状态机指针
 * @return 返回值数量（1个：true或nil+错误）
 */
i32 io_flush(LuaState* L);

/**
 * @brief io.input([file]) - 设置或获取默认输入文件
 *
 * 不带参数时返回当前输入文件，带参数时设置默认输入文件。
 *
 * @param L Lua状态机指针
 * @return 返回值数量（1个：文件句柄）
 */
i32 io_input(LuaState* L);

/**
 * @brief io.output([file]) - 设置或获取默认输出文件
 *
 * 不带参数时返回当前输出文件，带参数时设置默认输出文件。
 *
 * @param L Lua状态机指针
 * @return 返回值数量（1个：文件句柄）
 */
i32 io_output(LuaState* L);

/**
 * @brief io.type(obj) - 检查文件句柄类型
 *
 * 检查对象是否为文件句柄，返回 "file"、"closed file" 或 nil。
 *
 * @param L Lua状态机指针
 * @return 返回值数量（1个：字符串或nil）
 */
i32 io_type(LuaState* L);

/**
 * @brief io.lines([filename]) - 迭代文件行
 *
 * 返回一个迭代器函数，用于遍历文件的每一行。
 *
 * @param L Lua状态机指针
 * @return 返回值数量（1个：迭代器函数）
 */
i32 io_lines(LuaState* L);

/**
 * @brief io.tmpfile() - 创建临时文件
 *
 * 创建一个临时文件并返回文件句柄。
 *
 * @param L Lua状态机指针
 * @return 返回值数量（1个：文件句柄或nil+错误）
 */
i32 io_tmpfile(LuaState* L);

/**
 * @brief io.popen(prog [, mode]) - 打开管道执行程序
 *
 * 打开一个管道来执行外部程序，返回文件句柄用于读取或写入。
 *
 * @param L Lua状态机指针
 * @return 返回值数量（1个：文件句柄或nil+错误）
 */
i32 io_popen(LuaState* L);

// =====================================================================
// 文件句柄方法声明
// =====================================================================

/**
 * @brief file:close() - 关闭文件句柄
 * @param L Lua状态机指针
 * @return 返回值数量（1个或3个）
 */
i32 f_close(LuaState* L);

/**
 * @brief file:read(...) - 从文件读取
 * @param L Lua状态机指针
 * @return 返回值数量（读取的值的数量）
 */
i32 f_read(LuaState* L);

/**
 * @brief file:write(...) - 写入文件
 * @param L Lua状态机指针
 * @return 返回值数量（1个：文件句柄或nil+错误）
 */
i32 f_write(LuaState* L);

/**
 * @brief file:flush() - 刷新文件缓冲区
 * @param L Lua状态机指针
 * @return 返回值数量（1个：true或nil+错误）
 */
i32 f_flush(LuaState* L);

/**
 * @brief file:seek([whence [, offset]]) - 设置文件位置
 * @param L Lua状态机指针
 * @return 返回值数量（1个：新位置或nil+错误）
 */
i32 f_seek(LuaState* L);

/**
 * @brief file:setvbuf(mode [, size]) - 设置缓冲模式
 * @param L Lua状态机指针
 * @return 返回值数量（1个：true或nil+错误）
 */
i32 f_setvbuf(LuaState* L);

/**
 * @brief file:lines() - 迭代文件行
 * @param L Lua状态机指针
 * @return 返回值数量（1个：迭代器函数）
 */
i32 f_lines(LuaState* L);

// =====================================================================
// 内部辅助函数声明
// =====================================================================

/**
 * @brief 创建文件句柄userdata
 * @param L Lua状态机指针
 * @param fp 文件指针
 * @return 创建的userdata
 */
Userdata* createFileHandle(LuaState* L, FILE* fp, bool isPipe = false, const char* path = nullptr,
                           bool ownsFile = true);

/**
 * @brief 关闭引用指定路径的已打开文件句柄
 *
 * 供 Windows 重命名或删除兼容路径使用；在这些路径中，CRT 会拒绝重命名或删除仍处于打开
 * 状态的文件。
 */
bool releaseFileHandlesForPath(LuaState* L, const char* path);

/**
 * @brief 检查并获取文件指针
 * @param L Lua状态机指针
 * @param idx 栈索引
 * @return 文件句柄元数据，如果无效抛出错误
 */
FileHandleData* checkFilePtr(LuaState* L, i32 idx);

/**
 * @brief 获取默认输入文件
 * @param L Lua状态机指针
 * @return 文件指针
 */
FILE* getDefaultInput(LuaState* L);

/**
 * @brief 获取默认输出文件
 * @param L Lua状态机指针
 * @return 文件指针
 */
FILE* getDefaultOutput(LuaState* L);

/**
 * @brief 设置默认输入文件
 * @param L Lua状态机指针
 * @param fp 文件指针
 */
void setDefaultInput(LuaState* L, FILE* fp);

/**
 * @brief 设置默认输出文件
 * @param L Lua状态机指针
 * @param fp 文件指针
 */
void setDefaultOutput(LuaState* L, FILE* fp);

/**
 * @brief 文件句柄垃圾回收函数
 * @param L Lua状态机指针
 * @return 返回值数量（0个）
 */
i32 io_gc(LuaState* L);

/**
 * @brief 文件句柄字符串化函数
 * @param L Lua状态机指针
 * @return 返回值数量（1个：字符串）
 */
i32 io_tostring(LuaState* L);

} // namespace Lua
