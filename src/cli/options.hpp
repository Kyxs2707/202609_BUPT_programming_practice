#pragma once
#include "diagnostic/failure.hpp"
#include <span>
#include <string>
#include <string_view>
namespace myb {
enum class RunMode { Run, Check, Help, Version };
struct Options {
    RunMode mode{RunMode::Run};
    std::string file;
};
Options parse_options(std::span<const std::string_view>);
inline constexpr std::string_view version_text = "myb 0.1.0 (language 0.2)\n";
inline constexpr std::string_view help_text =
    "Usage: myb [--check] [--] <file>\n"
    "       myb --help\n"
    "       myb --version\n"
    "M1 development subset: scalar expressions, print(value), write(value).\n";
} // namespace myb
