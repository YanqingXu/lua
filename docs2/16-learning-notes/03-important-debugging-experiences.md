# Important Debugging Experiences — 重要调试经验

> 记录在调试过程中学到的重要教训。

## 调试技巧

1. **先检查字节码**: 很多"VM bug"其实是 CodeGen 生成了错误的字节码
2. **最小复现**: 把几百行的失败脚本精简到 5-10 行
3. **对比官方**: `lua5.1` 和 `lua_app` 同时运行，对比输出
4. **Trace 是好朋友**: `--trace` 可以看到每条指令执行前后的寄存器状态

## 常见错误模式

- **base 指针失效**: 栈扩展后 base 指向了旧内存 → 需要 `refreshBase(L)`
- **RK 寻址错误**: 把常量当寄存器或反了
- **多返回值截断**: 表达式上下文中多返回值被截断为第一个
- **Upvalue 未关闭**: break/return 时漏掉了 OP_CLOSE
- **String 驻留不一致**: 同一字符串有两份 GCString (StringPool 未去重)

## 最好的学习方法

跟着 [01-execution-pipeline/08-full-trace-example.md](../01-execution-pipeline/08-full-trace-example.md) 用调试器单步执行一遍 `local x = 1; local function add(a,b) return a+b+x end; print(add(2,3))`。
