---
status: active
verified_against: LICENSE; THIRD_PARTY_NOTICES.md; CONTRIBUTING.md; SECURITY.md; .gitignore; .github/ISSUE_TEMPLATE/; .github/PULL_REQUEST_TEMPLATE.md; .github/workflows/ci.yml; .github/workflows/nightly.yml; .github/workflows/release.yml; docs/release/release-checklist.md; tools/check_release_readiness.ps1; tools/verify_release_governance.py; tools/test_verify_release_governance.py
last_checked: 2026-08-14
applies_to: 首次将 YanqingXu/lua 从私有仓库公开并进入 v0.1.0-rc.1 前
---

# 开源就绪与首次公开清单

本页区分已经完成的仓库清理、仍需仓库所有者在 GitHub 执行的设置，以及公开后必须在同一候选 SHA 上重新取得的证据。切换可见性、提交、推送、规则设置和发布都不是本地清理的一部分。

## 仓库所有者确认

2026-08-14，仓库所有者确认：

- 本项目自有代码的版权归属仓库所有者本人，并有权按根 `LICENSE` 的 MIT 条款公开；
- 仓库不包含商业机密或真实业务数据；
- 当前没有可承担独立审查的第二位审查者。

前两项关闭了版权和数据保密阻塞。第三项不阻止把源码设为 Public，但会阻止当前 release governance 合同接受任何 RC 或正式发布证明：`approved_by` 与 `independent_reviewer` 必须是不同账号，`time-limited-waiver` 也不豁免这一要求。不得用维护者的其他账号伪造独立审查。

## 已完成的公开前审计

2026-08-14 对当时 `main` 基线 `242ceaed1379c4507b13317b49cc333b54f6b177` 完成以下只读检查：

- 使用校验和验证的 Gitleaks 8.30.1，以 `--all --full-history` 扫描全部可达 Git 历史；未发现凭据。Git 对象完整性检查未发现损坏。
- 扫描 49 个仍保留的 Actions artifact，解压后共 8,699 个文件、约 199 MB；未发现凭据。
- 55 次 Actions run 中有 52 份日志可获取并完成扫描；未发现凭据。3 份 2026-05-06 至 2026-05-15 的早期 Copilot cloud agent 日志已无法通过 GitHub 获取，因此不能声称扫描过这三份已不可用日志。
- 检查历史文件名、文本历史、诊断 trace、Visual Studio 用户文件和历史 PDB；未发现私钥/证书类文件名、用户主目录绝对路径或明显真实业务标识。
- `.lua_history`、`*.vcxproj.user` 和仓库根 `trace-*.jsonl` 已从版本控制中移除并加入忽略规则。它们仍存在于历史提交中；由于审计未发现秘密或个人路径，本次不改写历史。
- Lua 官方测试与 Alien Signals 测试材料的来源、版本、许可证和修改范围已记录在 `THIRD_PARTY_NOTICES.md`，并纳入 SDK 安装包与发布制品验证。

在最终提交公开前，应对最终工作树再运行一次 secrets 扫描，并人工检查 `git diff --cached`。若任何后续扫描发现真实秘密，先吊销或轮换凭据，再决定是否使用 `git filter-repo` 改写历史；不要把改写历史当作凭据轮换的替代品。

## 切换公开前

- [x] 确认仓库不包含商业保密算法、客户集成、真实业务 trace 或不允许再分发的数据。
- [x] 确认 `LICENSE` 的权利人正确，仓库所有者有权按 MIT 许可项目自有代码。
- [ ] 保存仓库备份，并记录最终候选的完整 SHA。
- [ ] 检查现有 Actions artifact 的保留必要性；无用诊断 artifact 可在公开前由仓库所有者删除。
- [ ] 在 GitHub Settings → General 中将仓库改为 Public。公开后立即完成下一节规则，期间不要合并或创建版本 tag。

## 公开后立即建立规则

先在 GitHub Settings → Advanced Security 中启用 private vulnerability reporting，验证 `SECURITY.md` 中的私密报告链接可用；同时启用 public repository 可用的 secret scanning、push protection 和 Dependabot alerts。

为 `main` 创建 active branch ruleset，且不设置可绕过发布门禁的常规 bypass：

- 只能通过 Pull Request 合并。当前单维护者阶段先把 required approvals 设为 0，避免完全锁死维护路径；独立审查者加入后立即提升为至少 1 名非作者批准，并要求最后一次可审查 push 获得批准。
- 要求分支在合并前更新到最新 `main`，要求所有 review conversation 已解决。
- 要求最近一次 CI 中的全部 17 个 job：Windows MSBuild Debug/Release、Linux GCC Debug/Release、Linux Clang Debug/Release/address/undefined/thread、Windows/Linux allocator contract、Linux format/tidy、component coverage、runtime benchmark、libFuzzer，以及 Linux ARM64、macOS ARM64 portability。
- 禁止 force push 和分支删除；不要给管理员保留日常绕过路径。

另建一个匹配 `refs/tags/v*` 的 active tag ruleset，限制 tag 创建、更新和删除，只允许书面发布流程中的发布负责人创建 annotated tag。版本 tag 不移动、不复用。

规则生效后通过 GitHub API 或设置页重新读取规则，确认 enforcement 为 active，并关闭跟踪治理阻塞的 Issue。

## 在公开状态重新取得同 SHA 证据

- [ ] 邀请至少一名能够审查维护者自有改动的独立审查者，并把 `main` required approvals 从 0 提升为 1；在此之前不得生成 RC 治理批准。
- [ ] 对公开前最后一次候选 `push` CI 使用 GitHub 的 rerun 功能，使同一 SHA 的 17 个 job 在公开状态重新执行并全部成功。
- [ ] 手动运行 `Nightly endurance`，并等待同一 SHA 的下一次 scheduled Nightly；两者都必须满足 runtime/native-module soak 与六目标 fuzz 的正式时长。
- [ ] 手动运行 `Release candidate packages` 的 candidate-only 路径，版本输入为计划中的 `0.1.0-rc.N`，下载并复验三平台包、SBOM、manifest 和 SHA-256。
- [ ] 生成并审核 `LUA_RELEASE_GOVERNANCE_ATTESTATION`，绑定最终候选 SHA、独立审查记录和已经生效的 branch/tag ruleset。
- [ ] 按 [RC 与正式发布门禁](release-checklist.md)核对 exact-SHA evidence；在上述证据全部闭环前不创建 `v0.1.0-rc.1`。

若公开后产生任何修复提交，旧 SHA 的 CI、Nightly、artifact、治理批准和 candidate-only 包都不能继续作为新候选证据。
