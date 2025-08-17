# Lua 解释器测试指南

本指南介绍如何测试 Lua 解释器的各项功能，验证实现的正确性。

## 🚀 快速开始

### 编译项目
```bash
# Windows (使用 MSVC)
.\msvc_build.bat

# 或者使用其他构建系统
mkdir build && cd build
cmake .. && make
```

### 运行基础测试
```bash
# 运行基础功能测试
.\bin\lua.exe bin\script\basic_features_test.lua
```

## 🧪 测试分类

### 1. 基础功能测试

#### **算术运算测试**
```bash
# 创建测试文件 arithmetic_test.lua
echo 'print("10 + 5 =", 10 + 5)
print("10 - 3 =", 10 - 3)  
print("6 * 7 =", 6 * 7)
print("15 / 3 =", 15 / 3)' > arithmetic_test.lua

# 运行测试
.\bin\lua.exe arithmetic_test.lua
```

**期望输出**:
```
10 + 5 = 15
10 - 3 = 7
6 * 7 = 42
15 / 3 = 5
```

#### **变量和赋值测试**
```bash
# 创建测试文件 variables_test.lua
echo 'local a = 42
local b = "hello"
print("a =", a)
print("b =", b)
a = a + 8
print("a + 8 =", a)' > variables_test.lua

# 运行测试
.\bin\lua.exe variables_test.lua
```

**期望输出**:
```
a = 42
b = hello
a + 8 = 50
```

### 2. 控制流测试

#### **条件语句测试**
```bash
# 运行现有的基础功能测试，包含 if-then-else
.\bin\lua.exe bin\script\basic_features_test.lua
```

#### **For 循环测试**
```bash
# 简单循环测试
.\bin\lua.exe bin\script\for\simple_for_test.lua

# 中等范围循环测试  
.\bin\lua.exe bin\script\for\medium_range_for.lua

# 自定义循环测试
echo 'print("倒数循环:")
for i = 5, 1, -1 do
    print("倒数:", i)
end' > countdown_test.lua

.\bin\lua.exe countdown_test.lua
```

### 3. 函数系统测试

#### **基础函数测试**
```bash
# 简单函数测试
.\bin\lua.exe bin\script\function\debug_function_test.lua

# 带参数的函数测试
.\bin\lua.exe bin\script\function\simple_function_test.lua
```

#### **自定义函数测试**
```bash
# 创建复杂函数测试
echo 'function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

print("factorial(5) =", factorial(5))

function fibonacci(n)
    if n <= 2 then
        return 1
    else
        return fibonacci(n-1) + fibonacci(n-2)
    end
end

print("fibonacci(6) =", fibonacci(6))' > advanced_functions_test.lua

.\bin\lua.exe advanced_functions_test.lua
```

### 4. 综合测试

#### **完整程序测试**
```bash
# 运行综合测试程序
.\bin\lua.exe bin\script\comprehensive_test.lua
```

#### **自定义综合测试**
```bash
# 创建自己的综合测试
echo '-- 综合功能测试
print("=== 开始综合测试 ===")

-- 变量和计算
local x = 10
local y = 20
print("x + y =", x + y)

-- 循环计算
local sum = 0
for i = 1, x do
    sum = sum + i
end
print("1到", x, "的和 =", sum)

-- 函数定义
function max(a, b)
    if a > b then
        return a
    else
        return b
    end
end

-- 函数调用
local result = max(x, y)
print("max(", x, ",", y, ") =", result)

print("=== 综合测试完成 ===")' > my_comprehensive_test.lua

.\bin\lua.exe my_comprehensive_test.lua
```

## 🔍 调试和故障排除

### 常见问题

#### **1. 编译错误**
```bash
# 检查编译器版本
cl /?  # MSVC
gcc --version  # GCC

# 清理并重新编译
del /Q bin\*.exe
.\msvc_build.bat
```

#### **2. 运行时错误**
```bash
# 检查文件路径
dir bin\lua.exe
dir bin\script\

# 使用简单测试验证基础功能
echo 'print("Hello, World!")' > hello.lua
.\bin\lua.exe hello.lua
```

#### **3. 功能不工作**
```bash
# 逐步测试各个功能
echo 'print(1 + 1)' > test1.lua
.\bin\lua.exe test1.lua

echo 'local a = 5; print(a)' > test2.lua  
.\bin\lua.exe test2.lua

echo 'for i = 1, 3 do print(i) end' > test3.lua
.\bin\lua.exe test3.lua
```

### 性能测试

#### **循环性能测试**
```bash
echo 'local start = os.clock()
local sum = 0
for i = 1, 100000 do
    sum = sum + i
end
local finish = os.clock()
print("计算完成，用时:", finish - start, "秒")
print("结果:", sum)' > performance_test.lua

.\bin\lua.exe performance_test.lua
```

## 📊 测试结果验证

### 成功标准

#### **基础功能**
- ✅ 算术运算返回正确结果
- ✅ 变量赋值和访问正常
- ✅ 条件语句正确分支
- ✅ 循环正确执行和退出

#### **函数系统**  
- ✅ 函数定义不报错
- ✅ 函数调用执行正确
- ✅ 参数传递正确
- ✅ 基础返回值正确

#### **综合测试**
- ✅ 多特性组合程序正常运行
- ✅ 复杂逻辑正确执行
- ✅ 无崩溃或无限循环

### 失败诊断

如果测试失败，检查以下方面：

1. **编译问题**: 确保所有源文件正确编译
2. **路径问题**: 确保测试文件路径正确
3. **语法问题**: 确保 Lua 代码语法正确
4. **功能限制**: 检查是否使用了未实现的功能

## 🎯 测试建议

### 开发者测试流程

1. **每次修改后运行基础测试**
   ```bash
   .\bin\lua.exe bin\script\basic_features_test.lua
   ```

2. **针对修改的功能运行专项测试**
   ```bash
   # 如果修改了循环相关代码
   .\bin\lua.exe bin\script\for\simple_for_test.lua
   ```

3. **运行综合测试验证整体功能**
   ```bash
   .\bin\lua.exe bin\script\comprehensive_test.lua
   ```

### 用户验收测试

1. **创建自己的测试程序**
2. **验证所需功能是否正常工作**
3. **测试边界情况和错误处理**

---

**最后更新**: 2025年8月17日  
**状态**: 基础测试框架完整，核心功能验证通过 ✅
