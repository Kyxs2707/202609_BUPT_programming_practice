#pragma once
#include <cstdint>
#include <optional>
#include <string_view>
namespace myb::utf8 {
struct DecodedCodePoint {
    std::uint32_t value;
    std::size_t width;
};
std::optional<DecodedCodePoint> decode_one(std::string_view, std::size_t) noexcept;
std::optional<std::size_t> count_code_points(std::string_view) noexcept;
bool valid(std::string_view) noexcept;
} // namespace myb::utf8
