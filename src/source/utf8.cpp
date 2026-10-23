#include "source/utf8.hpp"
namespace myb::utf8 {
std::optional<DecodedCodePoint> decode_one(std::string_view s, std::size_t i) noexcept {
    if (i >= s.size())
        return {};
    auto first = static_cast<unsigned char>(s[i]);
    if (first < 0x80)
        return DecodedCodePoint{first, 1};
    std::size_t width;
    std::uint32_t value, min;
    if (first >= 0xc2 && first <= 0xdf) {
        width = 2;
        value = first & 31;
        min = 0x80;
    } else if (first >= 0xe0 && first <= 0xef) {
        width = 3;
        value = first & 15;
        min = 0x800;
    } else if (first >= 0xf0 && first <= 0xf4) {
        width = 4;
        value = first & 7;
        min = 0x10000;
    } else
        return {};
    if (s.size() - i < width)
        return {};
    for (std::size_t j = 1; j < width; ++j) {
        auto c = static_cast<unsigned char>(s[i + j]);
        if ((c & 0xc0) != 0x80)
            return {};
        value = (value << 6) | (c & 63);
    }
    if (value < min || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
        return {};
    return DecodedCodePoint{value, width};
}
std::optional<std::size_t> count_code_points(std::string_view s) noexcept {
    std::size_t count = 0;
    for (std::size_t i = 0; i < s.size(); ++count) {
        auto d = decode_one(s, i);
        if (!d)
            return {};
        i += d->width;
    }
    return count;
}
bool valid(std::string_view s) noexcept {
    return count_code_points(s).has_value();
}
} // namespace myb::utf8
