# Return — 返回语句

## 1. RETURN 指令

```
RETURN A B
返回 B-1 个值，从 R(A) 开始

详见: 05-vm-runtime/08-return-values.md
```

## 2. Return 与作用域关闭

```
return 编译时:
  1. 如果 return 离开的作用域中有 upvalue → 先发出 OP_CLOSE
  2. 发出 RETURN 指令

return 必须是块的最后一条语句 (语法限制):
  function f()
     return 1  -- ok
     print("never")  -- 不可达
  end
```

## 3. Return + Break 的交互

```lua
-- Lua 5.1: return 必须是块的最后一条
do return end  -- ok (在 do-end 块中)
-- return 1, break  -- 非法
```
