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

- 提交信息除了前缀（例如docs,feat,fix）外，其余部分使用中文，只提交本次任务涉及的文件。

- 保留用户未提交的改动，构建生成物留在 `build/` 且不提交。

## 代码注释规范

- 新增或修改的手写 C/C++ 公共接口、跨模块接口、核心类型及具有复杂约束的内部接口，使用 Doxygen 文档注释；注释随实现同步更新。
- 文档注释统一使用 `/** ... */` 和 `@` 标签；简单成员说明可使用 `///<`。
- 说明文字使用中文，标识符保持源码拼写；注释遵守文档规则与禁用词表。
- 使用 `@brief` 概述职责；按需使用 `@param[in]`、`@param[out]`、`@param[in,out]`、`@tparam` 和 `@return` 说明参数、模板参数与返回结果。
- 按实际契约使用 `@pre`、`@post`、`@throws`；仅描述实现与已批准设计保证的条件、异常和副作用。
- 涉及所有权、借用、GC、临时根和调用交接时，明确有效期、保活责任、可能触发回收的操作及引用失效条件。
- 接口注释放在声明处；仅有定义时放在定义处。声明与定义不重复维护同一份说明。
- 实现内使用普通注释解释算法选择、检查顺序和清理原因；避免逐行复述代码。
- `void` 函数、构造函数和析构函数省略 `@return`；省略无实际内容的标签。

## 设计推进方式

- 按主题记录讨论，区分已确认决定、候选建议和后续扩展。
- 已有决定不重复询问；修订时同步规范与记录。
- 用完整小程序检验方案；新增能力说明用途、实现与测试成本。
- 对比限定 Python 实现、JVM 实现和 JavaScript 引擎。
- 核实其他实现的采用情况，引用官方资料并标明推断。
