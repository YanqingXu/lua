# Behavior Differences — 与官方 Lua 5.1 的行为差异

## 1. Number 类型

```
官方 Lua 5.1: number 默认是 double (可编译为 float/int)
本项目:      number 固定为 double (f64)
```

## 2. Table 遍历顺序

```
官方 Lua 5.1: 不保证 pairs 遍历顺序
本项目:       HashMap 遍历顺序取决于实现 (也不保证)

建议: 不要依赖 pairs 的遍历顺序。
```

## 3. #t (表长度) 行为

```
官方 Lua 5.1: 有洞的表的 #t 是"任意满足条件的边界"
本项目:       使用二分搜索，结果可能与官方不同但均在合法范围内

合法范围: 任意 i 满足 t[i] != nil 且 t[i+1] == nil
```

## 4. String Interning

```
官方 Lua 5.1: 短字符串 (≤40) 驻留，长字符串不驻留
本项目:       所有字符串都尝试驻留 (简化实现)

影响: 长字符串比较使用指针比较 (更快的 ==，但可能:
  - 创建大量唯一长字符串时内存使用更高
  - string 比较可能不等价于 strcmp)
```

## 5. Error Messages

```
官方 Lua:
  [string "source"]:3: attempt to ...

本项目:
  错误消息格式类似，但具体措辞可能有差异
```

## 6. GC 行为

```
官方 Lua 5.1: 增量式 GC (incremental)
本项目:       标记-清除 (mark-sweep)，IncrementalGC 为教学占位

影响: 本项目 GC 暂停时间可能更长 (stop-the-world)
```

## 7. Binary Chunk

```
官方 Lua 5.1: 特定二进制格式 (luac)
本项目:       本地格式，不与官方兼容

影响: string.dump() 的输出不能用官方 luac 加载
     loadstring() 不能加载官方 luac 的输出
```
