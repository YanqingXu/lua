# Design Decisions — 设计决策记录

> 记录项目中做出的重要设计决策和背后的考虑。

## 1. Value 使用 std::variant

**决策**: 使用 C++17 `std::variant` 而非 C 的 union

**原因**:
- 类型安全，不会访问错误的 union 成员
- 自动管理类型标签
- 支持复杂类型 (std::string, 指针)
- 调试友好 (Watch 窗口可以直接看到类型和值)

## 2. VM 使用 goto reentry 模式

**决策**: 使用 `goto reentry` 而非递归调用

**原因**:
- 避免 C++ 调用栈溢出
- 与 Lua 5.1 C 实现的设计一致
- 尾调用优化自然实现

## 3. StringPool 单例

**决策**: StringPool 使用单例模式

**原因**:
- 全局唯一的字符串驻留点
- 指针比较替代内容比较
- 简化 GC 逻辑

## 4. GC 使用策略模式

**决策**: GCStrategy 抽象接口 + MarkSweepGC / IncrementalGC 实现

**原因**:
- 可以切换不同的 GC 算法
- 教学友好的对比
- 未来可扩展为增量式 / 并发 GC

## 5. 自研测试框架

**决策**: 不使用 Google Test，自研轻量框架

**原因**:
- 零外部依赖
- 足够直接，便于理解
- 适合解释器这种底层项目
