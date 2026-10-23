#include "test_hooks.hpp"
#include "test_support.hpp"
#include <array>
#include <cmath>
#include <fast_float/fast_float.h>
#include <limits>
#include <ryu/ryu.h>
#include <span>
#include <string_view>
#include <system_error>

namespace myb {
MYB_TEST(toolchain) {
    REQUIRE(__cplusplus == 202002L);
    REQUIRE(std::numeric_limits<double>::is_iec559);
    REQUIRE(std::numeric_limits<double>::digits == 53);
    REQUIRE(std::numeric_limits<double>::max_exponent == 1024);
    REQUIRE(sizeof(double) == 8);
    std::array<double, 1> storage{1.25};
    std::span<const double> view(storage);
    REQUIRE(view[0] == 1.25);
    double number = 0;
    constexpr std::string_view text = "1.25";
    const auto result = fast_float::from_chars(text.data(), text.data() + text.size(), number);
    REQUIRE(result.ec == std::errc{});
    REQUIRE(result.ptr == text.data() + text.size());
    REQUIRE(number == 1.25);
    std::array<char, 32> buffer{};
    const int length = d2s_buffered_n(number, buffer.data());
    double restored = 0;
    const auto back = fast_float::from_chars(buffer.data(), buffer.data() + length, restored);
    REQUIRE(back.ec == std::errc{});
    REQUIRE(back.ptr == buffer.data() + length);
    REQUIRE(restored == number);
    constexpr std::string_view tiny = "-1e-9999";
    const auto underflow = fast_float::from_chars(tiny.data(), tiny.data() + tiny.size(), number);
    REQUIRE(underflow.ec == std::errc::result_out_of_range);
    REQUIRE(underflow.ptr == tiny.data() + tiny.size());
    REQUIRE(number == 0.0);
    REQUIRE(std::signbit(number));
    constexpr std::string_view large = "1e9999";
    const auto overflow = fast_float::from_chars(large.data(), large.data() + large.size(), number);
    REQUIRE(overflow.ec == std::errc::result_out_of_range);
    REQUIRE(std::isinf(number));
}

MYB_TEST(test_hooks) {
    TestHooks::clear_fault();
    TestHooks::fail_next(FaultPoint::Payload);
    TestHooks::hit(FaultPoint::HeapObject);
    bool failed = false;
    try {
        TestHooks::hit(FaultPoint::Payload);
    } catch (const std::bad_alloc &) {
        failed = true;
    }
    REQUIRE(failed);
    TestHooks::hit(FaultPoint::Payload);
    TestHooks::force_gc(true);
    REQUIRE(TestHooks::gc_forced());
    TestHooks::force_gc(false);
    REQUIRE(!TestHooks::gc_forced());
}
} // namespace myb
