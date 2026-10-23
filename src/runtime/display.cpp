#include "runtime/display.hpp"
#include "runtime/decimal_codec.hpp"
#include "runtime/object.hpp"
#include "runtime/text_builder.hpp"
#include <charconv>
namespace myb {
void display(Value value, TextBuilder &output, ErrorSite site) {
    std::visit(
        [&](auto data) {
            using T = decltype(data);
            if constexpr (std::is_same_v<T, NullValue>)
                output.append(std::span<const char>("null", 4));
            else if constexpr (std::is_same_v<T, bool>) {
                const std::string_view text = data ? "true" : "false";
                output.append(std::span<const char>(text.data(), text.size()));
            } else if constexpr (std::is_same_v<T, std::int64_t>) {
                char buffer[32];
                auto converted = std::to_chars(buffer, buffer + sizeof buffer, data);
                if (converted.ec != std::errc{})
                    fail(site, FailureDomain::Internal, ReasonCode::Invariant,
                         "integer display failed");
                output.append(std::span<const char>(
                    buffer, static_cast<std::size_t>(converted.ptr - buffer)));
            } else if constexpr (std::is_same_v<T, double>) {
                auto text = DecimalCodec::format_float(data);
                output.append(std::span<const char>(text.data(), text.size()));
            } else if constexpr (std::is_same_v<T, StringRef>) {
                output.append(std::span<const char>(data.ptr->utf8.data(), data.ptr->utf8.size()));
            } else {
                fail(site, FailureDomain::Internal, ReasonCode::Invariant,
                     "container display is outside M1");
            }
        },
        value.data);
}
} // namespace myb
