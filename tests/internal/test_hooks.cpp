#include "test_hooks.hpp"
#include <optional>

namespace myb::TestHooks {
namespace {
bool forced_gc = false;
std::optional<FaultPoint> next_failure;
} // namespace
void force_gc(bool enabled) noexcept {
    forced_gc = enabled;
}
bool gc_forced() noexcept {
    return forced_gc;
}
void fail_next(FaultPoint point) noexcept {
    next_failure = point;
}
void clear_fault() noexcept {
    next_failure.reset();
}
void hit(FaultPoint point) {
    if (next_failure == point) {
        next_failure.reset();
        throw std::bad_alloc{};
    }
}
} // namespace myb::TestHooks
