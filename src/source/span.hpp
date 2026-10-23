#pragma once
#include <cstdint>
namespace myb {
/** @brief SourceManager 内从零开始的源码记录编号。 */
using SourceId = std::uint32_t;
/**
 * @brief 原始 UTF-8 源文件中的字节半开范围 [begin, end)。
 *
 * 偏移包含文件开头 BOM 所占的字节；空范围可表示 EOF。范围本身不持有源码。
 */
struct SourceSpan {
    SourceId source{}; ///< 所属源码记录。
    std::uint64_t begin{}, end{}; ///< 起始字节偏移与结束字节偏移。
    friend bool operator==(const SourceSpan &, const SourceSpan &) = default;
};
/** @brief 从 1 开始的源码行列；列按 Unicode 码点计数，Tab 计一列。 */
struct SourcePosition {
    std::uint64_t line{}, column{};
};
} // namespace myb
