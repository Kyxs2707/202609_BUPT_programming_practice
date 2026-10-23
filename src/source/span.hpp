#pragma once
#include <cstdint>
namespace myb {
using SourceId = std::uint32_t;
struct SourceSpan {
    SourceId source{};
    std::uint64_t begin{}, end{};
    friend bool operator==(const SourceSpan &, const SourceSpan &) = default;
};
struct SourcePosition {
    std::uint64_t line{}, column{};
};
} // namespace myb
