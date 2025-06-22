# Modern C++ Lua Interpreter Wiki

欢迎来到Modern C++ Lua Interpreter项目的Wiki文档！这里包含了项目的详细技术文档、开发指南和使用说明。

## 📚 目录

- [项目概述](#项目概述)
- [快速开始](#快速开始)
- [架构设计](#架构设计)
- [核心模块](#核心模块)
- [开发指南](#开发指南)
- [API文档](#api文档)
- [性能优化](#性能优化)
- [测试指南](#测试指南)
- [故障排除](#故障排除)
- [贡献指南](#贡献指南)

## 项目概述

### 🎯 项目目标

Modern C++ Lua Interpreter是一个使用现代C++17技术重新实现的Lua解释器，旨在：

- **性能优化**：提供比原版Lua更高的执行效率
- **代码质量**：使用现代C++特性确保代码的安全性和可维护性
- **模块化设计**：清晰的架构便于扩展和维护
- **完全兼容**：保持与Lua 5.1语法和语义的完全兼容

### 🏆 核心特性

- ✅ **完整的Lua语言支持**：所有Lua语法结构
- ✅ **现代C++实现**：C++17标准，智能指针，RAII
- ✅ **高性能虚拟机**：基于寄存器的字节码执行引擎
- ✅ **先进垃圾回收**：三色标记清除算法
- ✅ **全面测试覆盖**：90%+的代码覆盖率
- ✅ **模块化架构**：清晰的组件分离

## 快速开始

### 环境要求

- **编译器**：支持C++17的编译器（GCC 7+, Clang 5+, MSVC 2017+）
- **构建系统**：Make, Ninja, Visual Studio 或其他构建系统
- **操作系统**：Windows, Linux, macOS

### 编译步骤

```bash
# 克隆仓库
git clone https://github.com/YanqingXu/lua.git
cd lua

# 使用您首选的构建系统编译项目
# 例如使用 Visual Studio、Make 或其他 IDE
```

### 运行示例

```bash
# 交互式模式
./lua

# 执行脚本
./lua script.lua

# 执行代码
./lua -e "print('Hello, World!')"
```

## 架构设计

### 🏗️ 整体架构

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Source Code   │───▶│     Lexer       │───▶│     Parser      │
│   (.lua files)  │    │  (Tokenization) │    │  (AST Builder)  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                                        │
                                                        ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Virtual Machine│◀───│    Compiler     │◀───│       AST       │
│   (Execution)   │    │  (Code Gen)     │    │   (Syntax Tree) │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │
         ▼
┌─────────────────┐
│ Garbage Collector│
│ (Memory Mgmt)   │
└─────────────────┘
```

### 📦 模块组织

#### 核心模块
- **lexer/**: 词法分析器，将源代码转换为Token流
- **parser/**: 语法分析器，构建抽象语法树(AST)
- **compiler/**: 编译器，将AST编译为字节码
- **vm/**: 虚拟机，执行字节码指令
- **gc/**: 垃圾回收器，自动内存管理

#### 支持模块
- **common/**: 公共定义和工具
- **lib/**: 标准库实现
- **tests/**: 测试套件

## 核心模块

### 🔤 词法分析器 (Lexer)

#### 功能概述
词法分析器负责将Lua源代码分解为Token序列，是编译过程的第一步。

#### 主要特性
- **完整Token支持**：关键字、标识符、字面量、操作符
- **错误处理**：详细的词法错误报告
- **位置跟踪**：精确的行号和列号信息
- **Unicode支持**：UTF-8字符串处理

#### 核心类型
```cpp
enum class TokenType {
    // 字面量
    NUMBER, STRING, BOOLEAN, NIL,
    
    // 标识符和关键字
    IDENTIFIER, KEYWORD,
    
    // 操作符
    PLUS, MINUS, MULTIPLY, DIVIDE,
    EQUAL, NOT_EQUAL, LESS, GREATER,
    
    // 分隔符
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    
    // 特殊
    EOF, NEWLINE
};

struct Token {
    TokenType type;
    std::string lexeme;
    std::any literal;
    int line;
    int column;
};
```

#### 使用示例
```cpp
#include "lexer/lexer.hpp"

Lua::Lexer lexer("local x = 42");
auto tokens = lexer.tokenize();

for (const auto& token : tokens) {
    std::cout << token.lexeme << " (" << static_cast<int>(token.type) << ")\n";
}
```

### 🌳 语法分析器 (Parser)

#### 功能概述
语法分析器将Token序列转换为抽象语法树(AST)，表示程序的语法结构。

#### AST节点类型

##### 表达式节点
```cpp
// 基础表达式接口
class Expression {
public:
    virtual ~Expression() = default;
    virtual void accept(Visitor& visitor) = 0;
};

// 字面量表达式
class LiteralExpression : public Expression {
    Value value;
public:
    LiteralExpression(Value val) : value(std::move(val)) {}
    void accept(Visitor& visitor) override;
};

// 二元运算表达式
class BinaryExpression : public Expression {
    Ptr<Expression> left;
    Token operator_;
    Ptr<Expression> right;
public:
    BinaryExpression(Ptr<Expression> l, Token op, Ptr<Expression> r)
        : left(std::move(l)), operator_(op), right(std::move(r)) {}
    void accept(Visitor& visitor) override;
};
```

##### 语句节点
```cpp
// 基础语句接口
class Statement {
public:
    virtual ~Statement() = default;
    virtual void accept(Visitor& visitor) = 0;
};

// 表达式语句
class ExpressionStatement : public Statement {
    Ptr<Expression> expression;
public:
    ExpressionStatement(Ptr<Expression> expr) : expression(std::move(expr)) {}
    void accept(Visitor& visitor) override;
};

// 局部变量声明
class LocalStatement : public Statement {
    Vec<Str> names;
    Vec<Ptr<Expression>> initializers;
public:
    LocalStatement(Vec<Str> n, Vec<Ptr<Expression>> init)
        : names(std::move(n)), initializers(std::move(init)) {}
    void accept(Visitor& visitor) override;
};
```

#### 解析策略
- **递归下降**：清晰的语法规则映射
- **优先级处理**：正确的运算符优先级
- **错误恢复**：语法错误后的恢复机制
- **左递归消除**：避免无限递归

### ⚙️ 编译器 (Compiler)

#### 功能概述
编译器将AST转换为虚拟机可执行的字节码指令序列。

#### 编译阶段
1. **AST遍历**：使用访问者模式遍历语法树
2. **代码生成**：为每个节点生成对应的字节码
3. **优化**：常量折叠、死代码消除等
4. **符号解析**：变量和函数的作用域处理

#### 指令集架构
```cpp
enum class OpCode : u8 {
    // 数据移动
    LOAD_CONST,     // 加载常量到寄存器
    LOAD_GLOBAL,    // 加载全局变量
    STORE_GLOBAL,   // 存储全局变量
    LOAD_LOCAL,     // 加载局部变量
    STORE_LOCAL,    // 存储局部变量
    
    // 算术运算
    ADD, SUB, MUL, DIV, MOD, POW,
    
    // 比较运算
    EQ, NE, LT, LE, GT, GE,
    
    // 逻辑运算
    AND, OR, NOT,
    
    // 控制流
    JUMP,           // 无条件跳转
    JUMP_IF_FALSE,  // 条件跳转
    CALL,           // 函数调用
    RETURN,         // 函数返回
    
    // 表操作
    NEW_TABLE,      // 创建新表
    GET_TABLE,      // 表索引读取
    SET_TABLE,      // 表索引设置
};

struct Instruction {
    OpCode opcode;
    u8 a, b, c;     // 操作数
    i32 sbx;        // 有符号扩展操作数
};
```

#### 寄存器分配
```cpp
class RegisterManager {
    std::vector<bool> used_registers;
    int next_register = 0;
    
public:
    int allocate() {
        for (int i = 0; i < used_registers.size(); ++i) {
            if (!used_registers[i]) {
                used_registers[i] = true;
                return i;
            }
        }
        used_registers.push_back(true);
        return used_registers.size() - 1;
    }
    
    void release(int reg) {
        if (reg < used_registers.size()) {
            used_registers[reg] = false;
        }
    }
};
```

### 🖥️ 虚拟机 (VM)

#### 功能概述
虚拟机是字节码的执行引擎，负责指令解释和程序状态管理。

#### 执行模型
- **基于寄存器**：减少栈操作，提高性能
- **直接线程化**：使用computed goto优化指令分发
- **调用栈管理**：函数调用和返回的高效处理

#### 核心组件
```cpp
class VirtualMachine {
    std::vector<Value> registers;      // 寄存器数组
    std::vector<CallFrame> call_stack; // 调用栈
    std::vector<Value> constants;      // 常量表
    GarbageCollector* gc;              // 垃圾回收器
    
public:
    void execute(const std::vector<Instruction>& code);
    Value call_function(Function* func, const std::vector<Value>& args);
};

struct CallFrame {
    Function* function;     // 当前函数
    int pc;                // 程序计数器
    int base_register;     // 寄存器基址
    int return_address;    // 返回地址
};
```

#### 指令执行循环
```cpp
void VirtualMachine::execute(const std::vector<Instruction>& code) {
    int pc = 0;
    
    while (pc < code.size()) {
        const Instruction& inst = code[pc];
        
        switch (inst.opcode) {
            case OpCode::LOAD_CONST:
                registers[inst.a] = constants[inst.b];
                break;
                
            case OpCode::ADD:
                registers[inst.a] = add_values(registers[inst.b], registers[inst.c]);
                break;
                
            case OpCode::JUMP:
                pc += inst.sbx;
                continue;
                
            case OpCode::CALL:
                call_function(registers[inst.a], inst.b, inst.c);
                break;
                
            // ... 其他指令
        }
        
        ++pc;
    }
}
```

### 🗑️ 垃圾回收器 (GC)

#### 功能概述
垃圾回收器自动管理内存，回收不再使用的对象，防止内存泄漏。

#### 算法设计
采用**三色标记清除算法**：
- **白色**：未访问的对象，可能是垃圾
- **灰色**：已访问但子对象未完全扫描的对象
- **黑色**：已完全扫描的活跃对象

#### 核心实现
```cpp
class GarbageCollector {
    std::unordered_set<GCObject*> all_objects;  // 所有对象
    std::stack<GCObject*> gray_stack;           // 灰色对象栈
    size_t bytes_allocated = 0;                 // 已分配字节数
    size_t next_gc = 1024 * 1024;              // 下次GC阈值
    
public:
    void collect_garbage();
    void mark_roots();
    void trace_references();
    void sweep();
};

void GarbageCollector::collect_garbage() {
    // 标记阶段
    mark_roots();
    trace_references();
    
    // 清除阶段
    sweep();
    
    // 调整下次GC阈值
    next_gc = bytes_allocated * 2;
}
```

#### GC对象基类
```cpp
class GCObject {
    GCColor color = GCColor::WHITE;
    
public:
    virtual ~GCObject() = default;
    virtual void mark_references(GarbageCollector* gc) = 0;
    
    GCColor get_color() const { return color; }
    void set_color(GCColor c) { color = c; }
};
```

## 开发指南

### 🛠️ 开发环境设置

#### IDE配置
推荐使用以下IDE之一：
- **Visual Studio Code** + C++ Extension
- **CLion**
- **Visual Studio 2019/2022**
- **Qt Creator**

#### 代码格式化
项目使用`.clang-format`配置文件统一代码风格：
```bash
# 格式化单个文件
clang-format -i src/lexer/lexer.cpp

# 格式化整个项目
find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

#### 静态分析
```bash
# 使用clang-tidy进行静态分析
clang-tidy src/**/*.cpp -- -std=c++17

# 使用cppcheck
cppcheck --enable=all --std=c++17 src/
```

### 📝 编码规范

#### 命名约定
- **类名**：PascalCase (`GarbageCollector`)
- **函数名**：snake_case (`collect_garbage`)
- **变量名**：snake_case (`bytes_allocated`)
- **常量**：UPPER_SNAKE_CASE (`MAX_STACK_SIZE`)
- **成员变量**：snake_case，私有成员可选下划线前缀

#### 文件组织
```cpp
// header.hpp
#pragma once

#include <system_headers>
#include "project_headers.hpp"

namespace Lua {
    class MyClass {
    private:
        // 私有成员
        
    public:
        // 公有接口
    };
}

// implementation.cpp
#include "header.hpp"

#include <additional_headers>

namespace Lua {
    // 实现
}
```

#### 错误处理
```cpp
// 使用异常处理错误
class LuaException : public std::exception {
    std::string message;
    
public:
    LuaException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

// 在代码中使用
void some_function() {
    if (error_condition) {
        throw LuaException("Detailed error message");
    }
}
```

### 🔧 构建系统

#### 构建配置
项目支持多种构建系统，请根据您的环境选择合适的构建方式：

- **Visual Studio**: 打开项目文件夹或解决方案文件
- **Make**: 使用 Makefile 进行编译
- **Ninja**: 快速并行构建
- **其他 IDE**: 导入源代码并配置编译选项

## API文档

### 🔌 公共API

#### State类 - Lua状态管理
```cpp
class State {
public:
    State();  // 创建新的Lua状态
    ~State(); // 清理资源
    
    // 栈操作
    void push(const Value& value);           // 压入值
    Value pop();                             // 弹出值
    Value& get(int idx);                     // 获取栈中值
    void set(int idx, const Value& value);   // 设置栈中值
    int get_top() const;                     // 获取栈顶位置
    
    // 类型检查
    bool is_nil(int idx) const;
    bool is_boolean(int idx) const;
    bool is_number(int idx) const;
    bool is_string(int idx) const;
    bool is_table(int idx) const;
    bool is_function(int idx) const;
    
    // 类型转换
    bool to_boolean(int idx) const;
    double to_number(int idx) const;
    std::string to_string(int idx) const;
    
    // 全局变量
    void set_global(const std::string& name, const Value& value);
    Value get_global(const std::string& name);
    
    // 代码执行
    bool do_string(const std::string& code);     // 执行字符串
    bool do_file(const std::string& filename);   // 执行文件
    
    // 函数调用
    Value call(const Value& func, const std::vector<Value>& args);
};
```

#### Value类 - Lua值系统
```cpp
class Value {
public:
    // 构造函数
    Value();                              // nil值
    Value(bool b);                        // 布尔值
    Value(double n);                      // 数字值
    Value(const std::string& s);          // 字符串值
    Value(std::shared_ptr<Table> t);      // 表值
    Value(std::shared_ptr<Function> f);   // 函数值
    
    // 类型查询
    ValueType get_type() const;
    bool is_nil() const;
    bool is_boolean() const;
    bool is_number() const;
    bool is_string() const;
    bool is_table() const;
    bool is_function() const;
    
    // 值提取
    bool as_boolean() const;
    double as_number() const;
    std::string as_string() const;
    std::shared_ptr<Table> as_table() const;
    std::shared_ptr<Function> as_function() const;
    
    // 运算符重载
    Value operator+(const Value& other) const;
    Value operator-(const Value& other) const;
    Value operator*(const Value& other) const;
    Value operator/(const Value& other) const;
    bool operator==(const Value& other) const;
    bool operator<(const Value& other) const;
};
```

#### Table类 - Lua表
```cpp
class Table : public GCObject {
public:
    Table();
    
    // 基本操作
    Value get(const Value& key) const;
    void set(const Value& key, const Value& value);
    bool has(const Value& key) const;
    void remove(const Value& key);
    
    // 迭代器支持
    class iterator {
    public:
        std::pair<Value, Value> operator*() const;
        iterator& operator++();
        bool operator!=(const iterator& other) const;
    };
    
    iterator begin();
    iterator end();
    
    // 元表支持
    void set_metatable(std::shared_ptr<Table> mt);
    std::shared_ptr<Table> get_metatable() const;
    
    // 大小和容量
    size_t size() const;
    size_t array_size() const;
    size_t hash_size() const;
    
    // GC接口
    void mark_references(GarbageCollector* gc) override;
};
```

### 🔧 扩展API

#### 自定义函数注册
```cpp
// C++函数包装
using LuaFunction = std::function<Value(const std::vector<Value>&)>;

// 注册C++函数到Lua
void register_function(State* state, const std::string& name, LuaFunction func) {
    auto lua_func = std::make_shared<NativeFunction>(func);
    state->set_global(name, Value(lua_func));
}

// 使用示例
register_function(state, "print", [](const std::vector<Value>& args) -> Value {
    for (const auto& arg : args) {
        std::cout << arg.as_string() << "\t";
    }
    std::cout << std::endl;
    return Value(); // nil
});
```

#### 模块系统
```cpp
class Module {
public:
    virtual ~Module() = default;
    virtual void register_functions(State* state) = 0;
};

// 数学模块示例
class MathModule : public Module {
public:
    void register_functions(State* state) override {
        register_function(state, "math.sin", [](const std::vector<Value>& args) {
            if (args.size() != 1 || !args[0].is_number()) {
                throw LuaException("math.sin expects one number argument");
            }
            return Value(std::sin(args[0].as_number()));
        });
        
        register_function(state, "math.cos", [](const std::vector<Value>& args) {
            if (args.size() != 1 || !args[0].is_number()) {
                throw LuaException("math.cos expects one number argument");
            }
            return Value(std::cos(args[0].as_number()));
        });
    }
};
```

## 性能优化

### ⚡ 编译时优化

#### 常量折叠
```cpp
// 编译时计算常量表达式
class ConstantFolder : public Visitor {
public:
    void visit_binary_expression(BinaryExpression* expr) override {
        expr->left->accept(*this);
        expr->right->accept(*this);
        
        // 如果两个操作数都是常量，直接计算结果
        if (is_constant(expr->left.get()) && is_constant(expr->right.get())) {
            Value left_val = evaluate_constant(expr->left.get());
            Value right_val = evaluate_constant(expr->right.get());
            
            if (expr->operator_.type == TokenType::PLUS) {
                Value result = left_val + right_val;
                replace_with_literal(expr, result);
            }
            // ... 其他运算符
        }
    }
};
```

#### 死代码消除
```cpp
// 移除永远不会执行的代码
class DeadCodeEliminator {
public:
    void eliminate(std::vector<Instruction>& code) {
        std::vector<bool> reachable(code.size(), false);
        
        // 标记可达指令
        mark_reachable(code, 0, reachable);
        
        // 移除不可达指令
        auto new_end = std::remove_if(code.begin(), code.end(),
            [&](const Instruction& inst) {
                size_t index = &inst - &code[0];
                return !reachable[index];
            });
        
        code.erase(new_end, code.end());
    }
};
```

### 🚀 运行时优化

#### 内联缓存
```cpp
// 缓存属性访问以提高性能
class InlineCache {
    struct CacheEntry {
        Table* table;
        Value key;
        size_t hash_index;
        Value value;
    };
    
    std::vector<CacheEntry> cache;
    
public:
    Value get_cached(Table* table, const Value& key) {
        // 查找缓存
        for (const auto& entry : cache) {
            if (entry.table == table && entry.key == key) {
                return entry.value;
            }
        }
        
        // 缓存未命中，执行正常查找
        Value result = table->get(key);
        
        // 更新缓存
        if (cache.size() < MAX_CACHE_SIZE) {
            cache.push_back({table, key, 0, result});
        }
        
        return result;
    }
};
```

#### 字符串内化
```cpp
// 字符串内化以减少内存使用和提高比较性能
class StringIntern {
    std::unordered_set<std::string> interned_strings;
    
public:
    const std::string& intern(const std::string& str) {
        auto it = interned_strings.find(str);
        if (it != interned_strings.end()) {
            return *it;
        }
        
        auto result = interned_strings.insert(str);
        return *result.first;
    }
};
```

### 📊 性能分析

#### 性能计数器
```cpp
class PerformanceCounters {
    std::unordered_map<std::string, uint64_t> counters;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> timers;
    
public:
    void increment(const std::string& name) {
        counters[name]++;
    }
    
    void start_timer(const std::string& name) {
        timers[name] = std::chrono::high_resolution_clock::now();
    }
    
    void end_timer(const std::string& name) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>
            (end_time - timers[name]).count();
        counters[name + "_time"] += duration;
    }
    
    void print_stats() const {
        for (const auto& [name, count] : counters) {
            std::cout << name << ": " << count << std::endl;
        }
    }
};
```

## 测试指南

### 🧪 测试框架

项目使用自定义的轻量级测试框架，支持单元测试和集成测试。

#### 基本测试结构
```cpp
#include "tests/test_main.hpp"

TEST_CASE("Lexer Basic Tokenization") {
    Lua::Lexer lexer("local x = 42");
    auto tokens = lexer.tokenize();
    
    ASSERT_EQ(tokens.size(), 4);
    ASSERT_EQ(tokens[0].type, Lua::TokenType::LOCAL);
    ASSERT_EQ(tokens[1].type, Lua::TokenType::IDENTIFIER);
    ASSERT_EQ(tokens[1].lexeme, "x");
    ASSERT_EQ(tokens[2].type, Lua::TokenType::ASSIGN);
    ASSERT_EQ(tokens[3].type, Lua::TokenType::NUMBER);
}

TEST_CASE("Parser Expression Parsing") {
    Lua::Lexer lexer("1 + 2 * 3");
    auto tokens = lexer.tokenize();
    
    Lua::Parser parser(tokens);
    auto expr = parser.parse_expression();
    
    ASSERT_NOT_NULL(expr);
    // 验证AST结构
}
```

#### 测试宏定义
```cpp
#define TEST_CASE(name) \
    void test_##name(); \
    static TestRegistrar reg_##name(#name, test_##name); \
    void test_##name()

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            throw TestFailure("Assertion failed: " #a " == " #b); \
        } \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw TestFailure("Assertion failed: " #condition); \
        } \
    } while(0)
```

### 📋 测试分类

#### 单元测试
测试单个组件的功能：
- **lexer_test.cpp**: 词法分析器测试
- **parser_test.cpp**: 语法分析器测试
- **compiler_test.cpp**: 编译器测试
- **vm_test.cpp**: 虚拟机测试
- **gc_test.cpp**: 垃圾回收器测试

#### 集成测试
测试组件间的协作：
- **end_to_end_test.cpp**: 完整的编译执行流程
- **compatibility_test.cpp**: Lua兼容性测试
- **performance_test.cpp**: 性能基准测试

#### 回归测试
确保修改不会破坏现有功能：
- **regression_test.cpp**: 已修复bug的测试用例

### 🎯 测试覆盖率

#### 生成覆盖率报告
```bash
# 使用gcov生成覆盖率数据
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make
ctest

# 生成HTML报告
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

#### 覆盖率目标
- **总体覆盖率**: > 90%
- **核心模块覆盖率**: > 95%
- **关键路径覆盖率**: 100%

## 故障排除

### 🐛 常见问题

#### 编译错误

**问题**: `error: 'std::variant' is not a member of 'std'`
**解决**: 确保使用C++17编译器并设置正确的标准
```bash
g++ -std=c++17 ...
# 或在CMake中
set(CMAKE_CXX_STANDARD 17)
```

**问题**: 链接错误 `undefined reference`
**解决**: 检查库依赖和链接顺序
```
# 根据您的构建系统链接所需的库
```

#### 运行时错误

**问题**: 段错误 (Segmentation Fault)
**调试步骤**:
1. 使用调试器运行
```bash
gdb ./lua
(gdb) run script.lua
(gdb) bt  # 查看调用栈
```

2. 启用地址消毒器
```bash
# 在编译时添加地址消毒器标志
g++ -fsanitize=address ...
```

**问题**: 内存泄漏
**检测方法**:
```bash
# 使用Valgrind
valgrind --leak-check=full ./lua script.lua

# 使用AddressSanitizer
export ASAN_OPTIONS=detect_leaks=1
./lua script.lua
```

#### 性能问题

**问题**: 执行速度慢
**分析步骤**:
1. 使用性能分析器
```bash
# 使用perf
perf record ./lua script.lua
perf report

# 使用gprof
g++ -pg ...
./lua script.lua
gprof lua gmon.out
```

2. 检查GC频率
```cpp
// 在代码中添加GC统计
std::cout << "GC collections: " << gc->get_collection_count() << std::endl;
std::cout << "Bytes allocated: " << gc->get_bytes_allocated() << std::endl;
```

### 🔍 调试技巧

#### 启用调试输出
```cpp
#ifdef DEBUG
#define DEBUG_PRINT(x) std::cout << "[DEBUG] " << x << std::endl
#else
#define DEBUG_PRINT(x)
#endif

// 使用
DEBUG_PRINT("Executing instruction: " << opcode_name(inst.opcode));
```

#### 断言检查
```cpp
#include <cassert>

void some_function(Value* value) {
    assert(value != nullptr);
    assert(value->is_number());
    // 函数实现
}
```

#### 日志系统
```cpp
enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

class Logger {
    LogLevel min_level = LogLevel::INFO;
    
public:
    template<typename... Args>
    void log(LogLevel level, const std::string& format, Args... args) {
        if (level >= min_level) {
            printf(("[" + level_name(level) + "] " + format + "\n").c_str(), args...);
        }
    }
};

// 使用
Logger logger;
logger.log(LogLevel::DEBUG, "Compiling function %s", func_name.c_str());
```

## 贡献指南

### 🤝 如何贡献

#### 报告Bug
1. 在GitHub Issues中创建新issue
2. 提供详细的重现步骤
3. 包含系统信息和错误日志
4. 如果可能，提供最小重现示例

#### 提交功能请求
1. 描述功能的用途和价值
2. 提供详细的设计方案
3. 考虑向后兼容性
4. 讨论实现的复杂度

#### 代码贡献
1. Fork项目仓库
2. 创建功能分支
3. 实现功能并添加测试
4. 确保所有测试通过
5. 提交Pull Request

### 📋 代码审查清单

#### 功能性
- [ ] 功能按预期工作
- [ ] 边界条件处理正确
- [ ] 错误处理完善
- [ ] 性能影响可接受

#### 代码质量
- [ ] 遵循项目编码规范
- [ ] 代码清晰易读
- [ ] 适当的注释和文档
- [ ] 没有代码重复

#### 测试
- [ ] 包含充分的单元测试
- [ ] 测试覆盖率不降低
- [ ] 集成测试通过
- [ ] 性能测试无回归

#### 文档
- [ ] API文档更新
- [ ] 用户文档更新
- [ ] 变更日志更新
- [ ] 示例代码正确

### 🏆 贡献者认可

我们感谢所有贡献者的努力！贡献者将被列入：
- 项目README的贡献者列表
- 发布说明中的感谢名单
- 项目网站的贡献者页面

---

## 📞 获取帮助

如果您在使用过程中遇到问题，可以通过以下方式获取帮助：

- 📧 **邮件**: [support@lua-cpp.org](mailto:support@lua-cpp.org)
- 💬 **讨论区**: [GitHub Discussions](https://github.com/YanqingXu/lua/discussions)
- 🐛 **问题报告**: [GitHub Issues](https://github.com/YanqingXu/lua/issues)
- 📖 **文档**: [项目Wiki](https://github.com/YanqingXu/lua/wiki)

感谢您对Modern C++ Lua Interpreter项目的关注和支持！