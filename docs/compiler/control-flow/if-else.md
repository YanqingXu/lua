# If / Else — 条件分支

## 1. 字节码模式

```
if cond then
    trueBody
else
    falseBody
end

字节码:
  EQ/LT/LE/TEST  cond  false  → 条件不满足跳到 falseBody
  ... trueBody ...
  JMP → end
  ... falseBody ...
  (end:)
```

## 2. 示例

```lua
if x > 0 then
    print("positive")
elseif x < 0 then
    print("negative")
else
    print("zero")
end
```

```
伪字节码:
  LT 0 tmp x     ; tmp = (0 < x)?
  TEST tmp 0     ; if not tmp → skip to elseif
  JMP → elseif
  
  ; then 分支
  GETGLOBAL print
  LOADK "positive"
  CALL print 1 1
  JMP → end
  
  ; elseif 分支
  LT x 0 tmp     ; tmp = (x < 0)?
  TEST tmp 0     ; if not tmp → skip to else
  JMP → else
  
  GETGLOBAL print
  LOADK "negative"
  CALL print 1 1
  JMP → end
  
  ; else 分支
  GETGLOBAL print
  LOADK "zero"
  CALL print 1 1
  (end:)
```
