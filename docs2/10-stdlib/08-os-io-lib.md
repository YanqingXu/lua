# OS & I/O Library

## OS Library (11/11 函数)

| 函数 | 说明 |
|------|------|
| `os.clock()` | CPU 时间 |
| `os.date([fmt [, time]])` | 日期格式化 |
| `os.time([table])` | 时间戳 |
| `os.difftime(t1, t2)` | 时间差 |
| `os.execute([cmd])` | 执行系统命令 |
| `os.exit([code])` | 退出程序 |
| `os.getenv(varname)` | 环境变量 |
| `os.remove(filename)` | 删除文件 |
| `os.rename(old, new)` | 重命名 |
| `os.setlocale(locale [, category])` | 设置 locale |
| `os.tmpname()` | 临时文件名 |

## I/O Library (11/11 函数 + 7/7 方法)

### 函数
| 函数 | 说明 |
|------|------|
| `io.open(fname [, mode])` | 打开文件 |
| `io.close([file])` | 关闭文件 |
| `io.read(...)` | 读取 stdin |
| `io.write(...)` | 写入 stdout |
| `io.flush()` | 刷新输出 |
| `io.input([file])` | 设置输入 |
| `io.output([file])` | 设置输出 |
| `io.lines([fname])` | 逐行迭代 |
| `io.type(obj)` | 检测文件句柄 |
| `io.tmpfile()` | 临时文件 |
| `io.stderr/stdin/stdout` | 标准流 |

### File 方法
`file:read()`, `file:write()`, `file:close()`, `file:lines()`, `file:seek()`, `file:flush()`, `file:setvbuf()`
