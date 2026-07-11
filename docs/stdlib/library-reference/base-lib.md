# Base Library — 基础库

## 1. 函数列表

| 函数 | 说明 |
|------|------|
| `print(...)` | 打印到 stdout |
| `type(v)` | 返回类型字符串 |
| `tostring(v)` | 转为字符串 |
| `tonumber(e [, base])` | 转为数字 |
| `error(msg [, level])` | 抛出错误 |
| `assert(v [, msg])` | 断言 |
| `next(t [, idx])` | 表遍历 |
| `pairs(t)` | 遍历所有键值 |
| `ipairs(t)` | 遍历数组部分 |
| `rawget(t, k)` | 不触发 __index |
| `rawset(t, k, v)` | 不触发 __newindex |
| `rawequal(a, b)` | 不触发 __eq |
| `select(n, ...)` | 选择参数 |
| `pcall(f, ...)` | 保护调用 |
| `xpcall(f, err)` | 带错误处理 |
| `loadstring(str)` | 编译字符串 |
| `loadfile([file])` | 编译文件 |
| `dofile([file])` | 编译并执行 |
| `load(func [, name])` | 通用加载 |
| `getfenv([f])` | 获取环境 |
| `setfenv(f, env)` | 设置环境 |
| `getmetatable(t)` | 获取元表 |
| `setmetatable(t, mt)` | 设置元表 |
| `collectgarbage(opt)` | GC 控制 |
| `gcinfo()` | GC 信息 |
| `unpack(t [, i [, j]])` | 展开表 |
| `newproxy()` | 创建 proxy |
| `_G` | 全局表 |
| `_VERSION` | "Lua 5.1" |
## 2. print() 实现

```cpp
int base_print(lua_State* L) {
    i32 n = lua_gettop(L);
    for (i32 i = 1; i <= n; i++) {
        if (i > 1) printf("\t");  // tab 分隔
        printf("%s", lua_tostring(L, i));
    }
    printf("\n");
    return 0;
}
```

## 3. pcall / xpcall

```cpp
// pcall: 保护模式调用
// 错误被捕获，不传播到调用者
// 返回: true, results... 或 false, error_message

int base_pcall(lua_State* L) {
    // 1. 保存错误处理状态
    // 2. 调用目标函数
    // 3. 如果出错 → 返回 false, error_msg
    // 4. 如果成功 → 返回 true, results...
}
```
