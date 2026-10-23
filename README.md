# MYB

BUPT 2025–2026 学年程序设计实践：小型通用脚本语言与 C++20 字节码解释器，首版面向 Windows。

当前完成 **M1 最小执行链**：UTF-8 源码 → Flex/Bison → AST → 字面量检查 → 字节码 → 栈式 VM。支持标量表达式和 `print`、`write`，其余语言能力按 M2–M4 继续实施。

```powershell
& .\build\debug\myb.exe .\examples\hello.myb
# 输出 3
& .\build\debug\myb.exe --check .\examples\hello.myb
```

首次使用见 [构建与测试](docs/guides/构建与测试.md)。语言规则见 [语言规范](docs/specs/MYB语言规范.md)，实现安排见 [解释器设计](docs/specs/MYB解释器设计.md) 和 [实施计划](docs/plans/MYB解释器实施计划.md)，验收范围见 [测试报告](docs/reports/测试报告_v0.1.md)。

默认黑盒入口：

```powershell
python tools/run_blackbox.py --interpreter build/debug/myb.exe
```

开发改动位于 `dev` 或其派生分支；`main` 仅通过 PR 合并。

CI 使用同一脚本运行 Windows Debug/Release 干净构建与回归：

```powershell
pwsh -File ./tools/ci.ps1 -Configuration Debug -Msys2Root D:/msys64
pwsh -File ./tools/ci.ps1 -Configuration Release -Msys2Root D:/msys64
```

脚本需要 PowerShell 7+（`pwsh`）；MSYS2 安装根按实际环境传入。自动回归与远端合并条件的验收记录见 [CI 与合并规则](docs/rules/CI与合并规则.md)。
