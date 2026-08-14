# Runtime vulnerability policy

本政策只覆盖 Lua 解释器、公开 C ABI、binary chunk、sandbox、资源治理与原生模块生命周期中的实现缺陷；不把宿主应用或部署环境的问题归入本项目。

## 支持范围

| 版本 | 状态 |
|---|---|
| `main` / `0.1.x` 候选 | 接受报告，尚未承诺生产级长期支持 |
| 未标记的旧提交 | 仅在当前分支仍可复现时处理 |

## 私密报告

优先使用仓库的 [GitHub private vulnerability reporting](https://github.com/YanqingXu/lua/security/advisories/new)。如果该入口暂时不可用，可以创建一个不包含漏洞细节的普通 Issue，请求维护者建立私密沟通渠道；不要在公开 Issue、Pull Request、讨论或日志中提交利用样例、凭据或未修补细节。

报告应包含：

- 受影响提交、平台、编译器和配置；
- 最小复现输入或宿主代码；
- 实际结果、期望边界和可重复性；
- sanitizer/crash log、资源上限与是否涉及原生模块；
- 已知影响和任何临时缓解方式。

维护者应先确认收到，再完成复现、影响分级、修复、回归测试和协调披露。修复发布前不得把未公开利用细节写入普通 changelog。

## 明确边界

- `SandboxPolicy` 是脚本能力边界，不是恶意宿主或已加载原生代码的隔离层。
- instruction/deadline/cancellation 依赖 VM 检查点或原生 callback 协作轮询。
- callback allocator 尚未覆盖所有宿主/实现临时分配；生产部署仍需进程级 CPU/内存限制。
- 这些已记录限制本身不是漏洞；绕过声明的拒绝路径、越界、UAF、双重释放、隔离破坏或可控资源上限失效属于报告范围。
