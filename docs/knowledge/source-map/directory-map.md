---
status: current
verified_against: docs/architecture/overview.md; src/compiler/; src/core/; src/vm/; src/gc/; src/lib/; src/runtime/runtime_services.hpp
last_checked: 2026-06-13
applies_to: Chinese source directory map and module locator
---

# Directory Map — 源码目录地图

## 1. 这个模块解决什么问题？

回答：**每个目录是干什么的？我想改某个功能应该先去哪找？**

## 2. 顶层目录结构

```
lua/
├── src/                    # 核心源码（日常代码修改的主战场）
├── tests/                  # 测试代码
│   ├── unit/               # C++ 单元测试
│   └── lua/                # Lua 脚本测试
├── docs/                   # 按解释器模块组织的唯一技术文档根目录
├── examples/               # 示例 Lua 脚本
├── bin/                    # 编译批处理脚本
├── build/                  # CMake 构建输出
├── lua.slnx                # VS 解决方案
├── lua.vcxproj             # 核心静态库项目
├── lua_app.vcxproj         # 解释器/REPL 项目
├── lua_test.vcxproj        # 测试项目
├── lua_bytecode.vcxproj    # 字节码工具项目
└── CMakeLists.txt          # CMake 构建配置
```

## 3. src/ 详细结构

```
src/
├── common/                 # 基础类型、配置、宏定义
│   ├── types.hpp           # 所有类型别名 (i32, f64, Vec<T>, Str...)
│   ├── config.hpp          # 编译配置和常量
│   ├── macros.hpp          # 实用宏
│   ├── lua_error.hpp       # 错误类型定义
│   └── number_conversion.hpp # 数字转换工具
│
├── compiler/               # 编译器前端
│   ├── lexer/              # 词法分析器
│   │   ├── lexer.hpp/cpp   # Lexer 主类
│   │   └── lexer_cursor.hpp/cpp # 字符游标
│   ├── parser/             # 语法分析器
│   │   ├── parser.hpp/cpp  # Parser 主类
│   │   ├── parser_expr.cpp # 表达式解析
│   │   ├── parser_stmt.cpp # 语句解析
│   │   ├── parser_func.cpp # 函数定义解析
│   │   ├── parser_primary.cpp # 基本表达式
│   │   ├── parser_table.cpp # 表构造器
│   │   └── token.hpp       # Token 定义
│   ├── codegen/            # 字节码生成
│   │   ├── codegen.cpp     # 编译入口
│   │   ├── codegen_stmt.cpp # 语句编译
│   │   ├── expression_emitter.cpp # 表达式字节码发射
│   │   ├── statement_emitter.cpp # 语句字节码发射
│   │   ├── function_compiler.cpp # 函数级编译
│   │   ├── scope_manager.cpp # 作用域管理
│   │   ├── jump_patcher.cpp # 跳转回填
│   │   ├── codegen_binding.cpp # 符号绑定
│   │   └── name_binder.cpp  # 名称绑定
│   ├── ast.hpp/cpp         # AST 节点定义
│   ├── ast_visitor.hpp     # AST 访问者
│   └── opcode.hpp/cpp      # 指令集定义 (38条指令)
│
├── core/                   # 运行时核心对象
│   ├── value.hpp/cpp       # Value 类 (std::variant)
│   ├── table.hpp/cpp       # Table 类 (数组+哈希)
│   ├── function.hpp/cpp    # Proto/Closure/Function
│   ├── upvalue.hpp/cpp     # Upvalue (Open/Closed)
│   ├── userdata.hpp/cpp    # Userdata (C 数据包装)
│   ├── gc_object.hpp/cpp   # GCObject 基类 (三色标记)
│   ├── gc_string.hpp/cpp   # GC 字符串
│   ├── string_pool.hpp/cpp # 字符串驻留池
│   ├── metatable.hpp/cpp   # 元方法系统
│   └── thread.hpp/cpp      # 协程
│
├── vm/                     # 虚拟机执行引擎
│   ├── vm.hpp/cpp          # VM 入口和主循环
│   ├── vm_entry.cpp        # executeProto 入口
│   ├── vm_loop.cpp         # 主循环实现
│   ├── vm_frame.cpp        # 调用帧管理
│   ├── vm_call.cpp         # 函数调用
│   ├── vm_arith.cpp        # 算术运算
│   ├── vm_table.cpp        # 表操作
│   ├── vm_ops.cpp          # 操作辅助
│   ├── vm_trace.cpp        # 追踪/调试
│   ├── vm_handlers.cpp     # Handler 注册
│   ├── vm_handlers/        # 各指令 Handler 实现
│   │   ├── vm_handlers_arith.cpp
│   │   ├── vm_handlers_branch.cpp
│   │   ├── vm_handlers_call.cpp
│   │   ├── vm_handlers_closure.cpp
│   │   ├── vm_handlers_data.cpp
│   │   ├── vm_handlers_global_upvalue.cpp
│   │   ├── vm_handlers_loop.cpp
│   │   ├── vm_handlers_table.cpp
│   │   └── vm_handlers_unary.cpp
│   ├── vm_dispatch_strategy.cpp # 分发策略
│   ├── state/              # 状态管理
│   │   ├── lua_state.hpp/cpp # LuaState
│   │   ├── global_state.hpp/cpp # GlobalState
│   │   ├── stack.hpp/cpp   # Stack
│   │   └── call_info.hpp   # CallInfo
│   └── vm_internal.hpp     # 内部类型
│
├── gc/                     # 垃圾回收
│   ├── garbage_collector.hpp/cpp # GC 主类
│   ├── gc_strategy.hpp/cpp # GC 策略
│   ├── gc_mark.cpp         # 标记阶段
│   ├── gc_sweep.cpp        # 清除阶段
│   ├── gc_finalize.cpp     # 终结器
│   └── gc_weak.cpp         # 弱表处理
│
├── lib/                    # 标准库
│   ├── baselib.cpp         # 基础库 (print, type, pcall...)
│   ├── mathlib.cpp         # 数学库
│   ├── stringlib.cpp       # 字符串库
│   ├── tablelib.cpp        # 表库
│   ├── oslib.cpp           # OS 库
│   ├── iolib.cpp           # I/O 库
│   ├── coroutinelib.cpp    # 协程库
│   ├── debuglib.cpp        # 调试库
│   ├── packagelib.cpp      # 包/模块库
│   ├── testlib.cpp         # 测试库
│   ├── lib_catalog.cpp     # 库目录
│   ├── lib_manager.cpp     # 库管理
│   └── lib_registry.cpp    # 库注册
│
├── io/                     # I/O 系统
│   ├── input_stream.hpp/cpp # 输入流
│   ├── file_loader.cpp     # 文件加载
│   └── dynamic_buffer.cpp  # 动态缓冲区
│
├── api/                    # C API
│   └── lapi.cpp            # Lua C API 实现
│
├── app/                    # 应用层
│   └── app_options.cpp     # 命令行选项
│
├── debug/                  # 调试工具
│   ├── value_serializer.cpp # 值序列化
│   └── json_trace_sink.cpp  # JSON 追踪输出
│
├── bytecode/               # 字节码工具
│   ├── bytecode_main.cpp   # 字节码工具入口
│   └── bytecode_printer.cpp # 字节码打印
│
├── repl/                   # REPL 子系统
│   ├── repl_comp.cpp       # Tab 补全
│   ├── repl_ctx.cpp        # REPL 上下文
│   ├── repl_exe.cpp        # REPL 执行
│   ├── repl_hist.cpp       # 历史记录
│   ├── repl_meta.cpp       # 元命令 (.bytecode, .ast, .gc)
│   ├── repl_prompt.cpp     # 提示符
│   └── repl_sig.cpp        # 信号处理
│
├── main.cpp                # lua_app 入口
├── repl.cpp                # REPL 公共入口
├── lua.h                   # 公共 C API 头
├── lauxlib.h               # 辅助库头
└── lualib.h                # 标准库头
```

## 4. 子项目说明

| 项目 | 入口 | 产物 |
|------|------|------|
| **lua.vcxproj** | 无（静态库） | `lua.lib` |
| **lua_app.vcxproj** | `src/main.cpp` | `lua_app.exe` |
| **lua_test.vcxproj** | `tests/unit/framework/test_runner.cpp` | `lua_test.exe` |
| **lua_bytecode.vcxproj** | `src/bytecode/bytecode_main.cpp` | `lua_bytecode.exe` |
