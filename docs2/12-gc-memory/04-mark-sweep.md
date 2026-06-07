# Mark-Sweep — 标记-清除

## 1. 三色标记

```
White: 未访问，可能被回收
Gray:  已访问但未扫描子引用
Black: 已完全扫描，不会被回收

标记阶段:
  for each root:
    root.mark() → Gray
    
  while (hasGrayObjects()):
    obj = popGray()
    obj.markChildren() → 子对象变为 Gray
    obj.color = Black

清除阶段:
  for each object:
    if object.color == White:
      free(object)
    else if object.color == Black:
      object.color = White  // 准备下一轮
```

## 2. markChildren 示例

```cpp
void Table::markChildren(GarbageCollector& gc) {
    for (auto& v : array_)  markIfCollectable(gc, v);
    for (auto& [k, v] : hash_) {
        markIfCollectable(gc, k);
        markIfCollectable(gc, v);
    }
    if (metatable_) gc.markObject(metatable_);
}
```

## 3. 写屏障

```
当 Black 对象被修改 (写入新的 White 对象引用):
  1. 将 Black 对象重新标记为 Gray
  2. 或将新引用直接标记为 Gray

保守写屏障 (本项目):
  - Table 写入时: 如果新值是 White GC 对象 → 标记为 Gray
  - 元表设置时: 同上
  - 函数环境设置时: 同上
```
