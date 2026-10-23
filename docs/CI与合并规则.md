# CI 与合并规则

日期：2026-10-23。范围为 [实施计划](./MYB解释器实施计划.md) 的 CI-01/CI-02，M2–M4 未启动。

CI 自动运行与 GitHub 强制合并门禁分别验收。当前本地 Debug/Release 脚本已通过，远端运行待验收；**CI-02 受 GitHub 私有仓库套餐能力限制，尚未生效，整个 CI 阶段仍未完成**。

## 自动构建与回归

工作流 `.github/workflows/ci.yml` 名为 `MYB CI`，固定 job ID/显示名为 `windows-debug`、`windows-release`。对目标为 dev/main 的 PR、dev 的 push 和手工 workflow_dispatch 运行。不按文件路径跳过检查，文档变更同样触发；不运行 pull_request_target，权限仅为 contents: read。每个 job 在独立的 windows-2025 runner 上安装工具，限时 30 分钟，不缓存 build 或生成物，不自动合并、发布或部署。

MSYS2 采用 UCRT64，安装 GCC、原生 CMake/Ninja/Python 和 MSYS Flex/Bison。实际根路径由 setup-msys2 的 `msys2-location` 输出传入脚本，不假设 runner 的安装目录。runner 标签和工具包会更新，实际版本以每次日志为准；CMake configure 只验证仓库内 fast_float/Ryu 的 SHA-256，不联网拉取依赖。

实施时核验并固定的 Action：

| Action | 来源版本 | 完整提交 SHA |
|---|---|---|
| [actions/checkout](https://github.com/actions/checkout/releases/tag/v7.0.1) | v7.0.1 | `3d3c42e5aac5ba805825da76410c181273ba90b1` |
| [msys2/setup-msys2](https://github.com/msys2/setup-msys2/releases/tag/v2.33.0) | v2.33.0 | `ec48f7c5447b3140e2b088413ae3a55687bccb6e` |
| [actions/upload-artifact](https://github.com/actions/upload-artifact/releases/tag/v7.0.2) | v7.0.2 | `cf430e030ddbb5b0abf93d22962f4752f3646cd9` |

安装根输出及镜像依据：[setup-msys2 固定版 README](https://github.com/msys2/setup-msys2/blob/ec48f7c5447b3140e2b088413ae3a55687bccb6e/README.md#outputs)、[官方 windows-2025 镜像](https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-Readme.md)。

## 本地复现与证据

```powershell
pwsh -File ./tools/ci.ps1 -Configuration Debug -Msys2Root D:/msys64
pwsh -File ./tools/ci.ps1 -Configuration Release -Msys2Root D:/msys64
```

CI 脚本要求 PowerShell 7+，与 GitHub 的 `shell: pwsh` 一致；通过 `#Requires` 在旧 PowerShell 执行前明确报错，避免 5.1 把成功 unittest 的 stderr 报告误当失败。本机原生 CMake/Ninja 使用既有 `build/hosttools`，通过 PATH 提供；MSYS2 根参数只是本机示例。每次脚本先清理本配置生成目录，再配置/构建；日志见 `build/ci-logs/<debug|release>/ci.log`。各原生命令立即检查退出码，首个失败终止流程并原样返回；包装器用同一 unittest discover 得到非空 suite 后实际执行，不写死测试数量。

本次实际工具：GCC 15.2.0、Flex 2.6.4、Bison 3.8.2、CMake 4.1.0、Ninja 1.13.0、UCRT64 Python 3.12.12。

| 验证 | Debug | Release |
|---|---|---|
| 干净配置与并行构建 | 退出 0 | 退出 0，内部测试 OFF |
| 内部测试 | 26 通过 | 不构建/运行 |
| driver unittest | 9 通过 | 按计划归 Debug |
| CLI unittest | 7 通过，显式选择 Debug | 7 通过，显式选择 Release |
| 默认黑盒 | 25 通过 | 25 通过 |
| 无测试依赖核验 | 不适用 | 27 条编译命令，无测试钩子/测试目录依赖 |

`test_rejects_empty_case_list` 在修改驱动前实际失败：空 cases 返回 0 并输出 `0 passed, 0 failed, 0 total`；修复后返回 2，不打印 PASS。既有坏 golden、超时后继续、严格 UTF-8 和隔离测试继续通过。最初沙箱临时目录权限问题不算语义红测试，已在可访问临时目录的环境复验。

## GitHub 运行验收

待补充正常 → 错误 `three.stdout` → 恢复三个提交各自的两项 job 状态、运行链接、提交 SHA 与日志入口。远端 dev 仍停在早期文档提交，验证分支需要携带本地已有、已验收的 M1 基线；本次不改动解释器功能。

## 远端门禁现状与恢复步骤

仓库为私有仓库，默认分支 main；现有 Git 凭证对应账户具有 admin/maintain/push 权限。读取 dev/main 的 branch protection 与适用 rules 时，GitHub 均返回 HTTP 403：

```text
Upgrade to GitHub Pro or make this repository public to enable this feature.
```

因此不能核验、增补或验收强制保护。这是仓库套餐能力限制，不是缺少管理权限；没有修改仓库可见性、账户付费方案或现有远端保护。自动运行绿色结果不能证明失败会被 GitHub 禁止合并。

账户取得支持私有仓库保护的能力后，按以下步骤继续 CI-02：

1. 重新读取 dev/main 的现有 branch protection 和 rulesets，保存原始设置；读取实际 check runs 的精确名称及 GitHub Actions App ID。
2. 对 main 增补 require PR、`windows-debug`/`windows-release` required checks 与目标分支最新要求。对 dev 增补同两项 required checks 与最新要求，不额外要求所有更新走 PR。不降低原有审批、签名等更严格约束，不添加管理员/机器人绕过。
3. 若已有 branch protection，合并已有检查并优先 PATCH required_status_checks 子资源；若采用 ruleset，分别维护 main 的 pull_request 与 required_status_checks，以及 dev 的 required_status_checks。读回核对来源为 GitHub Actions。
4. 用错误 golden 分别验证目标 dev/main 的 PR 被必需检查阻止，main 验证 PR 不合并。恢复 golden 后核对最新提交两项检查通过；其他审批/冲突单独记录，不降低规则求绿。
5. 保存设置读回、失败阻止原因、恢复后状态及运行链接，完成 CI-02 后再将整个 CI 阶段标为完成。

dev 的状态检查要求同样约束直接更新；在本计划只为 dev push、dev/main PR 触发 CI 的安排下，日常推荐通过 PR 取得检查，不能假定未经检查的提交可直接推进 dev。未来如需派生分支直接通过检查再快进 dev，应另行扩展触发范围。

依据：[分支保护 API](https://docs.github.com/en/rest/branches/branch-protection)、[仓库 rules API](https://docs.github.com/en/rest/repos/rules)、[必需检查排障](https://docs.github.com/en/pull-requests/how-tos/merge-and-close-pull-requests/troubleshooting-required-status-checks)。
