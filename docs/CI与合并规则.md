# CI 与合并规则

日期：2026-10-23。范围为 [实施计划](./MYB解释器实施计划.md) 的 CI-01/CI-02，M2–M4 未启动。

CI 自动运行与 GitHub 强制合并门禁分别验收，**CI-01 已完成实现与验收**。本地 Debug/Release 脚本及远端正常→错误预期→恢复三阶段已验收；普通代码 PR 与独立的仅文档 PR 均已实际触发两项检查。**CI-02 受 GitHub 私有仓库套餐能力限制，尚未生效，整个 CI 阶段仍未完成**。

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

本机实际工具：GCC 15.2.0、Flex 2.6.4、Bison 3.8.2、CMake 4.1.0、Ninja 1.13.0、UCRT64 Python 3.12.12。远端 windows-2025 实测 GCC 16.2.0、Python 3.14.8、CMake 4.4.4、Ninja 1.13.2、Flex 2.6.4、Bison 3.8.2；以下同组测试也在远端通过，不将本机版本冒充 runner 固定版本。

| 验证 | Debug | Release |
|---|---|---|
| 干净配置与并行构建 | 退出 0 | 退出 0，内部测试 OFF |
| 内部测试 | 26 通过 | 不构建/运行 |
| driver unittest | 10 通过 | 按计划归 Debug |
| CLI unittest | 7 通过，显式选择 Debug | 7 通过，显式选择 Release |
| 默认黑盒 | 25 通过 | 25 通过 |
| 无测试依赖核验 | 不适用 | 27 条编译命令，无测试钩子/测试目录依赖 |

`test_rejects_empty_case_list` 在修改驱动前实际失败：空 cases 返回 0 并输出 `0 passed, 0 failed, 0 total`；修复后返回 2，不打印 PASS。既有坏 golden、超时后继续、严格 UTF-8 和隔离测试继续通过。最初沙箱临时目录权限问题不算语义红测试，已在可访问临时目录的环境复验。

## GitHub 运行验收

首轮 [运行 39875495711](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39875495711) 实际检出 CLI 自测路径表示差异：runner 的 MinGW Python 输出 `/` 路径，Windows 宿主诊断输出 `\`。已只修正测试的 native 路径预期，保持行列、错误类别、退出值与源码未被修改的断言；本机 Debug/Release 各 7 项 CLI 复验通过，远端最终复验通过。该次失败的两个 job 仍成功上传日志。同样的路径差异会影响黑盒 `$SCRIPT`；本机正斜杠路径探针实际复现 14/25 例路径匹配失败后，驱动改为在 Windows 上向解释器传入原生路径，并对相同参数转义生成占位符。未归一化实际输出或放宽诊断断言；探针、driver 自测与 Release 黑盒全部通过。补充持久回归 `test_forward_slash_pathlib_matches_windows_native_diagnostic`：对修复前驱动实际失败，对当前驱动通过；诊断子进程 fixture 始终按 Windows 原生路径报告，独立于 Python 的路径分隔符偏好。

代码验证 PR 为 [PR #1 → dev](https://github.com/Kyxs2707/202609_BUPT_programming_practice/pull/1)，携带远端尚未包含的已验收 M1 前置提交，本次不增加语言功能。

| 阶段 | 提交 SHA | windows-debug | windows-release | 运行及日志 |
|---|---|---|---|---|
| 正常 | `0070a4c76329d1e28e34203a6ddd7703bb335ebd` | success | success | [运行 39876394997](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39876394997) |
| 错误预期 | `20262fe442a5f210c9c46bc91bab8723f9b9b1b6` | failure | failure | [运行 39876719870](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39876719870) |
| 恢复 | `9787e6fb2b54604d4e52f73442ddbd75292fec46` | success | success | [运行 39877019675](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39877019675) |

三个阶段均实际生成 `windows-debug-logs` 与 `windows-release-logs`，可在各运行页 Artifacts 下载；完整 job 日志也在运行页。错误预期阶段仅把 `three.stdout` 从 `3\n` 改为 `4\n`，两 job 均报告 `24 passed, 1 failed, 25 total`、stdout 预期 4/实际 3，并由脚本保留退出 1。恢复提交的完整源码树与正常提交相同（`git diff 0070a4c 9787e6f --exit-code` 为 0），最终文件仍为原始 `3\n` 字节。实际检查名为 `windows-debug`、`windows-release`，来源 GitHub Actions App ID `15368`。

仅文档 PR 须在代码合入 dev 后独立建立，累计 diff 必须只有文档；不能将 PR #1 内的文档提交冒充仅文档 PR。PR #1 已按用户授权合并到 dev（`d016dc2f12c9a75efe5149f2222e8340e577f8a2`），main 保持 `b79e69dd931a98a7b103651b6d3ccbf6affe2f44`。本节文档收尾从新 dev 建立独立文档 PR，触发证据如下。代码 PR 合并前的最新文档提交也已在 [运行 39877521916](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39877521916) 两 job 全绿。


仅文档验收为 [PR #2 → dev](https://github.com/Kyxs2707/202609_BUPT_programming_practice/pull/2)。初始提交 `77ae9d5c8ee3688ad41c7a0709b78b861116e6d8` 的累计差异只有 `docs/CI与合并规则.md`，在 [运行 39877939248](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39877939248) 实际出现 `windows-debug`、`windows-release`，证明路径筛选没有省略文档检查。随后本 PR 只补充 Markdown 验收记录和计划状态；最新提交与结果以 PR 检查页为准。本 PR 保持开放供审阅。

合入 dev 的提交 `d016dc2f12c9a75efe5149f2222e8340e577f8a2` 也实际以 `push` 事件触发 [运行 39877856449](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39877856449)，两个固定 job 均出现。

## 远端门禁现状与恢复步骤

仓库为私有仓库，默认分支 main；现有 Git 凭证对应账户具有 admin/maintain/push 权限。读取 dev/main 的 branch protection 与适用 rules 时，GitHub 均返回 HTTP 403：

```text
Upgrade to GitHub Pro or make this repository public to enable this feature.
```

用户已明确选择保留私有仓库，先完成自动 CI，将门禁标为待完成。因此本轮不继续增补或验收强制保护。这是仓库套餐能力限制，不是缺少管理权限；没有修改仓库可见性、账户付费方案或现有远端保护。自动运行绿色结果不能证明失败会被 GitHub 禁止合并。

账户取得支持私有仓库保护的能力后，按以下步骤继续 CI-02：

1. 重新读取 dev/main 的现有 branch protection 和 rulesets，保存原始设置；读取实际 check runs 的精确名称及 GitHub Actions App ID。
2. 对 main 增补 require PR、`windows-debug`/`windows-release` required checks 与目标分支最新要求。对 dev 增补同两项 required checks 与最新要求，不额外要求所有更新走 PR。不降低原有审批、签名等更严格约束，不添加管理员/机器人绕过。
3. 若已有 branch protection，合并已有检查并优先 PATCH required_status_checks 子资源；若采用 ruleset，分别维护 main 的 pull_request 与 required_status_checks，以及 dev 的 required_status_checks。读回核对来源为 GitHub Actions。
4. 用错误 golden 分别验证目标 dev/main 的 PR 被必需检查阻止，main 验证 PR 不合并。恢复 golden 后核对最新提交两项检查通过；其他审批/冲突单独记录，不降低规则求绿。
5. 保存设置读回、失败阻止原因、恢复后状态及运行链接，完成 CI-02 后再将整个 CI 阶段标为完成。

dev 的状态检查要求同样约束直接更新；在本计划只为 dev push、dev/main PR 触发 CI 的安排下，日常推荐通过 PR 取得检查，不能假定未经检查的提交可直接推进 dev。未来如需派生分支直接通过检查再快进 dev，应另行扩展触发范围。

依据：[分支保护 API](https://docs.github.com/en/rest/branches/branch-protection)、[仓库 rules API](https://docs.github.com/en/rest/repos/rules)、[必需检查排障](https://docs.github.com/en/pull-requests/how-tos/merge-and-close-pull-requests/troubleshooting-required-status-checks)。
