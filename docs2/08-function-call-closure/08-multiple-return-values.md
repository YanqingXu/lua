# Multiple Return Values — 多返回值

> 详细内容见 [05-vm-runtime/08-return-values.md](../05-vm-runtime/08-return-values.md)

## 关键点

- `return a, b, c` → RETURN 指令的 B 参数控制返回值数量
- 多返回值在表达式中截断为第一个
- 函数实参末尾展开、表构造器末尾展开
- TAILCALL 传递所有返回值
- LUA_MULTRET (-1) 表示接受所有返回值
