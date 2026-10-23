# MYB 工作指南

## 项目定位

MYB 是面向数据处理和终端应用的小型通用脚本语言，直接运行源文件，普通变量动态绑定。首版支持 Windows，语言核心与平台操作分离。

## 项目环境

C++20、UCRT64 GCC、Flex/Bison、CMake/Ninja、Python 3.10+、PowerShell 7+。本机 MSYS2 位于 `D:\msys64`；工具路径、版本与初始化见构建文档。

## 必读文件

按任务阅读对应文件；编写或修改文档前先读文档规则与词表。

| 职责 | 文件 |
|---|---|
| 文档与用词 | [文档规则](docs/rules/文档构建原则.md)、[禁用词表](docs/forbidden_terms.yaml) |
| 功能与语义 | [原始需求](docs/requirements/原始需求文档.md)、[语言规范](docs/specs/MYB语言规范.md) |
| 解释器实现 | [解释器设计](docs/specs/MYB解释器设计.md) |
| 任务与进度 | [实施计划](docs/plans/MYB解释器实施计划.md)、[项目步骤](docs/superpowers/plans/2026-10-11-myb-implementation.md) |
| 构建与验收 | [构建文档](docs/guides/构建与测试.md)、[测试报告](docs/reports/测试报告_v0.1.md)、[CI 规则](docs/rules/CI与合并规则.md) |
| 决定追溯 | [语言讨论](docs/specs/discussions/MYB语言设计讨论记录.md)、[解释器讨论](docs/specs/discussions/MYB解释器设计讨论记录.md) |

## 执行顺序

1. 按最新用户指令确定范围，核对计划依赖和状态。
2. 按已确认规则实施；文档冲突先查确认记录。
3. 并行任务明确负责文件和交付内容，由主 Agent 整合。
4. 完成相应检查后更新记录，报告改动、验证结果和未完成项。

## 检查命令与通过标准

在仓库根目录执行；代码或构建改动运行 Debug、Release，纯文档改动检查链接、用词与差异格式。

```powershell
pwsh -File ./tools/ci.ps1 -Configuration Debug -Msys2Root D:/msys64
pwsh -File ./tools/ci.ps1 -Configuration Release -Msys2Root D:/msys64
git diff --check
```

通过标准：适用命令退出 0、测试全部通过、文档链接有效、词表零命中。PR 要求 `windows-debug`、`windows-release` 均成功；据实际结果标记任务完成。

## Git协作规则

- 编辑和提交前确认分支；改动位于 `dev` 或由 `dev` 创建的分支。
- `main` 仅通过 GitHub PR 更新，禁止直接修改、提交或推送。
- 提交信息使用中文，只提交本次任务涉及的文件。
- 保留用户未提交的改动，构建生成物留在 `build/` 且不提交。

## 设计推进方式

- 按主题记录讨论，区分已确认决定、候选建议和后续扩展。
- 已有决定不重复询问；修订时同步规范与记录。
- 用完整小程序检验方案；新增能力说明用途、实现与测试成本。
- 对比限定 Python 实现、JVM 实现和 JavaScript 引擎。
- 核实其他实现的采用情况，引用官方资料并标明推断。
