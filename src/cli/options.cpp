#include "cli/options.hpp"
namespace myb {
namespace {
[[noreturn]] void usage() {
    throw HostFailure({FailureDomain::Usage, ReasonCode::Syntax},
                      "expected myb [--check] [--] <file>; use --help");
}
} // namespace
Options parse_options(std::span<const std::string_view> args) {
    if (args.size() == 1 && args[0] == "--help")
        return {RunMode::Help, {}};
    if (args.size() == 1 && args[0] == "--version")
        return {RunMode::Version, {}};
    Options out;
    std::size_t at = 0;
    if (at < args.size() && args[at] == "--check") {
        out.mode = RunMode::Check;
        ++at;
    }
    if (at < args.size() && args[at] == "--")
        ++at;
    else if (at < args.size() && args[at].starts_with('-'))
        usage();
    if (at + 1 != args.size() || args[at].empty())
        usage();
    out.file = args[at];
    return out;
}
} // namespace myb
