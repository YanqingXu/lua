# Call Stack Dump — 调用栈追溯

## 1. debug.traceback()

```lua
-- 获取调用栈
local trace = debug.traceback("error message", 2)
print(trace)
-- 输出:
-- error message
-- stack traceback:
--     [string "main"]:5: in function 'foo'
--     [string "main"]:10: in function 'bar'
--     [string "main"]:15: in main chunk
```

## 2. 实现

```cpp
Str debug_traceback(lua_State* L, const char* msg, i32 level) {
    // 遍历 CallInfo 数组
    // 从当前帧往上回溯
    // 每帧输出: 文件名、行号、函数名
}
```

## 3. 行号信息

```
Proto 中的 lineInfo 数组:
  lineInfo[pc] = 源代码行号

通过 savedpc 可以反向查找行号。
```
