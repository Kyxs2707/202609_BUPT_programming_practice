#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace myb {
class EmergencyFailure;
enum class NumericParseError { None, Content, Range };
enum class ParseMode { SourceLiteral, Conversion };
template <class T> struct NumericParseResult {
    std::optional<T> value;
    NumericParseError error = NumericParseError::None;
};
class DecimalCodec {
  public:
    static void initialize(EmergencyFailure &);
    static NumericParseResult<double> parse_float(std::string_view, ParseMode);
    static NumericParseResult<std::int64_t> parse_int(std::string_view);
    static std::string format_float(double);
};
} // namespace myb
