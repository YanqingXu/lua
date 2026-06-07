# Entry Points — 程序入口

## 1. 这个模块解决什么问题？

说明项目中所有可执行程序的入口点和执行流程。

## 2. 三个入口

### lua_app.exe — 解释器/REPL

```
入口: src/main.cpp → main()

流程:
  main()
    ├── 解析命令行参数 (app_options)
    ├── 如果有 -e: 执行 -e 代码
    ├── 如果有文件: 加载并执行 .lua 文件
    ├── 如果是交互式 (-i): 进入 REPL
    ├── 如果是 stdin: 读取 stdin 并执行
    └── 如果无参数: 进入 REPL

REPL 模式:
  repl.run()
    ├── 显示提示符
    ├── 读取一行输入
    ├── 编译 → Proto
    ├── VM 执行
    ├── 打印结果
    └── 循环

元命令 (.help / .bytecode / .ast / .gc):
  repl_meta.cpp 处理
```

### lua_test.exe — 测试运行器

```
入口: tests/unit/framework/test_runner.cpp → main()

流程:
  main()
    ├── 解析命令行参数 (--max-memory-mb, --no-memory-limit)
    ├── 设置 512 MB 进程内存硬上限
    ├── 遍历所有注册的 TestSuite
    │   ├── 运行每个 TestCase
    │   └── 统计通过/失败
    └── 打印结果

测试注册:
  每个测试文件通过 TestRegistry 注册自己
  TestRegistry 是单例，启动时自动收集所有测试
```

### lua_bytecode.exe — 字节码工具

```
入口: src/bytecode/bytecode_main.cpp → main()

流程:
  main()
    ├── 加载 Lua 源文件
    ├── Parser → AST → CodeGen → Proto
    └── BytecodePrinter::print(proto, format)

输出格式:
  --format compact: 精简输出
  --format full: 完整输出（含常量表、局部变量）
  --diff file1 file2: 并排对比两个文件的字节码
  --cfg: Mermaid 控制流图输出
```
