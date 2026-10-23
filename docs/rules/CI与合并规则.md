# CI 与合并规则

日期：2026-10-23。范围为 [实施计划](../plans/MYB解释器实施计划.md) 的 CI-01/CI-02，M2–M4 未启动。

**CI-01、CI-02 均已完成实现与验收，M1 与 M2 之间的 CI 工程阶段完成。** 本地/远端自动回归、普通代码/仅文档 PR 触发、必需检查失败时的实际合并拦截均已验证。用户将仓库改为公开后完成 CI-02，随后取消 main 的 `Restrict updates`；当前仍须满足 PR、必需 CI、线性历史及扫描等要求。下文分别记录当前配置与原验收结果。

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

仅文档 PR 须在代码合入 dev 后独立建立，累计 diff 必须只有文档；不能将 PR #1 内的文档提交冒充仅文档 PR。PR #1 已按用户授权合并到 dev（`d016dc2f12c9a75efe5149f2222e8340e577f8a2`），验收时 main 保持 `b79e69dd931a98a7b103651b6d3ccbf6affe2f44`。本节文档收尾从新 dev 建立独立文档 PR，触发证据如下。代码 PR 合并前的最新文档提交也已在 [运行 39877521916](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39877521916) 两 job 全绿。


仅文档验收为 [PR #2 → dev](https://github.com/Kyxs2707/202609_BUPT_programming_practice/pull/2)。初始提交 `77ae9d5c8ee3688ad41c7a0709b78b861116e6d8` 的累计差异只有 `docs/CI与合并规则.md`，在 [运行 39877939248](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39877939248) 实际出现 `windows-debug`、`windows-release`，证明路径筛选没有省略文档检查。随后 PR #2 只补充 Markdown 验收记录和计划状态。PR #2 现已关闭、未合并，其全部提交已包含在 [PR #3 → dev](https://github.com/Kyxs2707/202609_BUPT_programming_practice/pull/3) 中；原 PR 和运行记录继续作为仅文档触发的验收证据。

合入 dev 的提交 `d016dc2f12c9a75efe5149f2222e8340e577f8a2` 也实际以 `push` 事件触发 [运行 39877856449](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39877856449)，两个固定 job 均出现。

## CI-02：远端强制门禁

CI-01 阶段仓库为私有，保护 API 曾返回要求 GitHub Pro/公开仓库的 403；2026-10-23 用户将仓库设为 public 并授权完成 CI-02。重新核验公开状态、默认分支 main 和 admin 权限后，读取并保存全部已有规则，再增补 CI 门禁。

### 当前生效配置（2026-10-23 复核）

新增 [MYB CI 必需检查 ruleset 25688063](https://github.com/Kyxs2707/202609_BUPT_programming_practice/rules/25688063)，enforcement 为 active，匹配 `refs/heads/dev`、`refs/heads/main`，无排除分支、无绕过名单：

| 项目 | 已读回的值 |
|---|---|
| 必需检查 | `windows-debug`、`windows-release` |
| 来源 | GitHub Actions，`integration_id: 15368` |
| 目标分支最新要求 | `strict_required_status_checks_policy: true` |
| 创建引用时豁免 | `do_not_enforce_on_create: false` |
| 绕过名单 | `bypass_actors: []`，当前管理账户也不可绕过 |

main 另有 [main ruleset 25686722](https://github.com/Kyxs2707/202609_BUPT_programming_practice/rules/25686722)，active、匹配默认分支、无绕过名单。用户在 CI-02 原验收结束后取消了 `Restrict updates`（`update`）；复核时该规则集的最后更新时间为 2026-10-23 09:36:10（UTC+08:00）。当前读回如下：

| main 规则 | 当前配置 |
|---|---|
| 必须通过 PR 更新 | 保留，普通 PR 的必需审批人数为 0 |
| 禁止删除、禁止强制推送、限制创建 | 保留 |
| Restrict updates（限制更新） | 已取消，不再以此规则锁住分支更新 |
| 线性历史 | 保留，PR 合并须使用 Squash 或 Rebase |
| Code Quality | 保留，阈值为 `warnings` |
| CodeQL | 保留，安全警报阈值 `high_or_higher`，其他警报阈值 `errors_and_warnings` |

两份规则集叠加生效：main 必须满足 PR、两项必需 CI、目标分支最新、线性历史和扫描等条件；取消 `Restrict updates` 不代表其他条件已经满足，具体合并状态以实际面向 main 的 PR 为准。dev 仍仅要求两项必需检查及目标分支最新，不额外要求 PR 或审批人数。

实施前原始 main 配置、精确新增请求、服务端读回、原配置 SHA-256、分支引用与逐项红绿验收保存在 [门禁配置与验收快照](../ci/merge-gate-evidence.json)。原 CI-02 实施只新增 CI 规则集，验收结束时 main 规则集与实施前逐项相等；用户后续取消 `Restrict updates` 的操作发生在该快照之后。快照保留原始验收事实，不作为当前配置副本，也不改写其中的 `update`、`original_main_ruleset_unchanged` 或合并状态；内容仅含公开配置与验证结果，不含凭证。

### 真实失败与恢复验证（原 CI-02 验收）

验证使用 dev 派生分支 `codex/myb-ci02-verification`，分别建立 [PR #3 → dev](https://github.com/Kyxs2707/202609_BUPT_programming_practice/pull/3) 和 [PR #4 → main](https://github.com/Kyxs2707/202609_BUPT_programming_practice/pull/4)，均非 draft。main PR 只读取门禁状态，恢复验证后已关闭、未合并。

| 目标 | 阶段与提交 | 两项必需检查 | GitHub 合并状态 | 运行 |
|---|---|---|---|---|
| dev | 错误预期 `b5caf23d098ec4a9f404fd601b5d5782bc5ef827` | 均 FAILURE、isRequired=true | BLOCKED | [39880489423](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39880489423) |
| main | 同一错误预期提交 | 均 FAILURE、isRequired=true | BLOCKED | [39880492344](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39880492344) |
| dev | 恢复 `a92d05a969377b7da8a3f769d5a62eb036e8458d` | 均 SUCCESS、isRequired=true | CLEAN | [39880946990](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39880946990) |
| main | 同一恢复提交 | 均 SUCCESS、isRequired=true | 当时为 BLOCKED，其他既有保护仍适用 | [39880947423](https://github.com/Kyxs2707/202609_BUPT_programming_practice/actions/runs/39880947423) |

排队阶段同样实际观测到两 PR 为 BLOCKED。失败阶段仅将 `three.stdout` 从 `3\n` 临时改为 `4\n`；四份 job 日志均确认 stdout 预期 4/实际 3、`24 passed, 1 failed, 25 total`、脚本退出 1，并成功上传失败日志。恢复后两组 Debug/Release 均全绿，原始 `3\n` 字节已恢复；`git diff dac9a19 a92d05a --exit-code` 为 0，临时错误差异不留在最终源码。

拦截证据包含 active ruleset 读回、GraphQL `CheckRun.isRequired(pullRequestNumber)` 为 true、检查实际失败和 `mergeStateStatus: BLOCKED`，不是仅凭 Actions 红色截图。`mergeable: MERGEABLE` 只说明无合并冲突，不能代替门禁状态。没有调用真实 merge API 或写入 main 试探保护。

**原验收中的 main CI 结果与其他保护分开判断。** 恢复后两项 CI 要求已满足，但 PR #4 当时总体仍为 BLOCKED；当时 main ruleset 包含 `update`（限制更新且无绕过）、线性历史、代码质量及 CodeQL 要求。该状态是原配置下的历史结果，不代表用户取消 `Restrict updates` 后的新 PR 状态。dev 当时恢复为 CLEAN，证明在仅有新增 CI 门禁的目标上，失败会阻止合并、成功后门禁条件满足。

原 CI-02 验收未更新 main/dev 分支内容；验收结束时 main 为 `b79e69dd931a98a7b103651b6d3ccbf6affe2f44`，dev 为 `d016dc2f12c9a75efe5149f2222e8340e577f8a2`。PR #3 汇总 CI-01/CI-02 文档供审阅，目标为 dev；最新提交及检查见 PR 页。

### 后续开发

dev 的必需检查同样约束直接更新。在当前只为 dev push、dev/main PR 触发 CI 的安排下，日常通过 PR 取得检查；不能假定未经检查的新提交可直接推进 dev。已有通过检查的同一 SHA 才能满足直接更新的状态要求。后续新增正式测试继续接入默认入口，不省略文档变更检查。

依据：[ruleset API](https://docs.github.com/en/rest/repos/rules)、[规则叠加](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/about-rulesets#about-rule-layering)、[CheckRun.isRequired](https://docs.github.com/en/graphql/reference/checks#checkrun)、[合并状态](https://docs.github.com/en/graphql/reference/pulls#mergestatestatus)。
