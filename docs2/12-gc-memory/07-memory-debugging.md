# Memory Debugging — 内存调试

## 1. GC 状态查看

```lua
-- 查看内存使用
print(collectgarbage("count"))  -- KB

-- 强制执行 GC
collectgarbage("collect")

-- REPL 中
> .gc  -- 查看 GC 状态
```

## 2. 内存泄漏排查

```
症状: 长时间运行后内存持续增长

排查步骤:
  1. 检查是否有意外的全局变量 (存在 _G 中)
  2. 检查 table 是否有循环引用 (GC 应该能处理)
  3. 检查 upvalue 是否意外持有大对象
  4. 检查 StringPool 中的字符串数量
  5. 使用 collectgarbage("count") 监控

工具:
  lua_app --trace → 查看对象生命周期
  Visual Studio 内存分析器 → C++ 侧排查
```

## 3. 内存限制

```
lua_test.exe 默认设置 512 MB 进程内存硬上限
可通过 --max-memory-mb <mb> 调整
或 --no-memory-limit 取消限制
```
