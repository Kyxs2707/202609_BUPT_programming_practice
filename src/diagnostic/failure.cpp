#include "diagnostic/failure.hpp"
namespace myb {
[[noreturn]] void fail(ErrorSite site, FailureDomain domain, ReasonCode reason,
                       std::string_view message, std::optional<std::uint32_t> os_code) {
    FailureHeader h{domain, reason, site.span, os_code};
    // 消息复制可能分配失败，须在构造详细异常前留下可决定退出状态的首错。
    site.failure.record_first(h);
    switch (domain) {
    case FailureDomain::Frontend:
    case FailureDomain::Structure:
        throw FrontendFailure(h, message);
    case FailureDomain::Runtime:
        throw MybRuntimeFailure(h, message);
    default:
        throw HostFailure(h, message);
    }
}
int exit_code(const FailureHeader &h) noexcept {
    switch (h.domain) {
    case FailureDomain::Usage:
    case FailureDomain::EntryIO:
        return 2;
    case FailureDomain::Resource:
    case FailureDomain::Internal:
        return 3;
    default:
        return 1;
    }
}
std::string_view domain_name(FailureDomain d) noexcept {
    switch (d) {
    case FailureDomain::Usage:
        return "usage error";
    case FailureDomain::EntryIO:
        return "entry I/O error";
    case FailureDomain::Frontend:
        return "frontend error";
    case FailureDomain::Structure:
        return "structure error";
    case FailureDomain::Runtime:
        return "runtime error";
    case FailureDomain::Resource:
        return "resource error";
    case FailureDomain::Internal:
        return "internal error";
    }
    return "internal error";
}
std::string_view reason_name(ReasonCode r) noexcept {
    switch (r) {
#define MYB_REASON(x)                                                                              \
    case ReasonCode::x:                                                                            \
        return #x;
        MYB_REASON(Encoding)
        MYB_REASON(Syntax) MYB_REASON(Declaration) MYB_REASON(Name) MYB_REASON(Uninitialized)
            MYB_REASON(Type) MYB_REASON(Range) MYB_REASON(Arithmetic) MYB_REASON(IterationMutation)
                MYB_REASON(IO) MYB_REASON(Assertion) MYB_REASON(Capacity) MYB_REASON(Allocation)
                    MYB_REASON(Invariant)
#undef MYB_REASON
    }
    return "Invariant";
}
} // namespace myb
