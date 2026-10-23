#pragma once
#include <cstdint>
#include <optional>
#include <string_view>
namespace myb::utf8 {
/** @brief 一个 Unicode 标量值及其 UTF-8 字节宽度。 */
struct DecodedCodePoint {
    std::uint32_t value;
    std::size_t width;
};
/**
 * @brief 从给定字节偏移严格解码一个 UTF-8 码点。
 * @return 码点及宽度；偏移越界、序列截断、过长编码或非法标量返回空值。
 */
std::optional<DecodedCodePoint> decode_one(std::string_view, std::size_t) noexcept;
/**
 * @brief 严格验证整个字节串并统计码点，保留 NUL 和 BOM 的普通码点含义。
 * @return 码点数量；空串返回 0，任何非法 UTF-8 序列返回空值。
 */
std::optional<std::size_t> count_code_points(std::string_view) noexcept;
/** @brief 检查整个字节串是否为合法 UTF-8，空串合法。 */
bool valid(std::string_view) noexcept;
} // namespace myb::utf8
