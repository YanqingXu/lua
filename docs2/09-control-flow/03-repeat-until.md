# Repeat-Until — 后置条件循环

## 1. 字节码模式

```
repeat
    body
until cond

字节码:
  (body_start:)  ← body 从这里开始
  ... body ...
  EQ/LT/LE/TEST  cond  true → JMP to body_start (继续循环)
  (end:)  ← 条件为真时退出
```

## 2. 与 While 的差异

```
While:  先检查条件 → 可能一次都不执行
Repeat: 先执行 body → 至少执行一次

While:   条件为真时继续
Repeat:  条件为真时退出 (until cond → cond 为真则停止)
```

## 3. Repeat 中的局部变量

```lua
-- 注意: repeat 体中声明的 local 在 until 条件中可见
repeat
    local x = getValue()
until x > 0  -- x 可见!
```
