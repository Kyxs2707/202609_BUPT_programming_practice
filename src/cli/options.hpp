#pragma once
#include "diagnostic/failure.hpp"
#include <span>
#include <string>
#include <string_view>
namespace myb {
/** @brief 一个命令的执行模式；Check 只编译和核验，Help/Version 只输出宿主文本。 */
enum class RunMode { Run, Check, Help, Version };
/** @brief 独占保存解析后的入口路径与模式，不借用命令行参数。 */
struct Options {
    RunMode mode{RunMode::Run};
    std::string file;
};
/**
 * @brief 解析不含程序名的 UTF-8 参数，接受一个入口文件及其前置 --check、--。
 *
 * --help 和 --version 须单独使用；-- 后的连字符路径按文件名处理。
 * @throws HostFailure 参数缺失、多余、为空或选项组合无效，分类为 Usage/Syntax。
 */
Options parse_options(std::span<const std::string_view>);
inline constexpr std::string_view version_text = "myb 0.1.0 (language 0.2)\n";
inline constexpr std::string_view help_text =
    "Usage: myb [--check] [--] <file>\n"
    "       myb --help\n"
    "       myb --version\n"
    "M1 development subset: scalar expressions, print(value), write(value).\n";
} // namespace myb
