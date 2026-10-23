#include "diagnostic/failure.hpp"
#include "runtime/decimal_codec.hpp"
#include "test_support.hpp"
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <string>

using namespace myb;

MYB_TEST(decimal_codec) {
    EmergencyFailure first;
    DecimalCodec::initialize(first);
    for (auto text : {".5", "1.", "1e", "NaN", "1", "+1.0", " 1.0", "0x1p0", "12abc"}) {
        REQUIRE(DecimalCodec::parse_float(text, ParseMode::SourceLiteral).error ==
                NumericParseError::Content);
    }
    for (auto text : {"", " ", "+.5", "1.", "inf", "1_0", "1e+", "1 2", "--1"}) {
        REQUIRE(DecimalCodec::parse_float(text, ParseMode::Conversion).error ==
                NumericParseError::Content);
    }
    REQUIRE(*DecimalCodec::parse_float(" +01e2 ", ParseMode::Conversion).value == 100.0);
    auto zero = DecimalCodec::parse_float("-1e-9999", ParseMode::Conversion);
    REQUIRE(zero.value && *zero.value == 0 && std::signbit(*zero.value));
    REQUIRE(DecimalCodec::parse_float("1e9999", ParseMode::Conversion).error ==
            NumericParseError::Range);
    REQUIRE(*DecimalCodec::parse_int("-9223372036854775808").value ==
            std::numeric_limits<std::int64_t>::min());
    REQUIRE(*DecimalCodec::parse_int(" +00012\t").value == 12);
    REQUIRE(DecimalCodec::parse_int("9223372036854775808").error == NumericParseError::Range);
    REQUIRE(DecimalCodec::parse_int("-9223372036854775809").error == NumericParseError::Range);
    REQUIRE(DecimalCodec::parse_int("1.0").error == NumericParseError::Content);
    REQUIRE(DecimalCodec::parse_int("1e0").error == NumericParseError::Content);

    struct ParseCase {
        const char *text;
        std::uint64_t bits;
    };
    const ParseCase parse_cases[] = {
        {"1.00000000000000011102230246251565404236316680908203125", 0x3ff0000000000000ULL},
        {"1.00000000000000033306690738754696212708950042724609375", 0x3ff0000000000002ULL},
        {"1.7976931348623157e308", 0x7fefffffffffffffULL},
        {"4.9406564584124654e-324", 1},
        {"9007199254740993", 0x4340000000000000ULL},
        {"9007199254740995", 0x4340000000000002ULL},
        {"-0.0", 0x8000000000000000ULL},
    };
    for (auto c : parse_cases) {
        auto parsed = DecimalCodec::parse_float(c.text, ParseMode::Conversion);
        REQUIRE(parsed.value && std::bit_cast<std::uint64_t>(*parsed.value) == c.bits);
    }
    struct FormatCase {
        double value;
        const char *text;
    };
    const FormatCase format_cases[] = {
        {0.0, "0.0"},
        {-0.0, "-0.0"},
        {1.0, "1.0"},
        {1e3, "1000.0"},
        {1e15, "1000000000000000.0"},
        {1e16, "1e+16"},
        {1e-4, "0.0001"},
        {1e-5, "1e-5"},
        {std::numeric_limits<double>::max(), "1.7976931348623157e+308"},
        {std::numeric_limits<double>::denorm_min(), "5e-324"},
        {std::nextafter(1e-4, 0.0), "9.999999999999999e-5"},
        {std::nextafter(1e16, 0.0), "9999999999999998.0"},
        {std::nextafter(1.0, 2.0), "1.0000000000000002"},
        {0.1, "0.1"},
        {2.98023223876953125e-8, "2.9802322387695312e-8"}};
    for (auto c : format_cases)
        REQUIRE(DecimalCodec::format_float(c.value) == c.text);
    std::mt19937_64 random(0x4d5942);
    for (unsigned i = 0; i < 10000; ++i) {
        auto bits = random();
        auto value = std::bit_cast<double>(bits);
        if (!std::isfinite(value))
            continue;
        auto parsed =
            DecimalCodec::parse_float(DecimalCodec::format_float(value), ParseMode::Conversion);
        REQUIRE(parsed.value && std::bit_cast<std::uint64_t>(*parsed.value) == bits);
    }
    REQUIRE(!first.is_set());
}
