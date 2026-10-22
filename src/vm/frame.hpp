#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
namespace myb {
struct Frame {
    std::uint32_t unit_id{};
    std::optional<std::uint32_t> function_id;
    std::size_t next_ip{}, executing_ip{}, base{}, operand_base{}, region_end{}, sp{};
    std::size_t caller_arg_base{}, caller_call_ip{}, iteration_base{};
};
} // namespace myb
