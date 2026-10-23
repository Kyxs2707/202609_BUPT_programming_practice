#pragma once
#include "runtime/operators.hpp"
#include <cstdint>
namespace myb {
enum class Op : std::uint16_t {
    CONST,
    DUP,
    DROP,
    DECL_LOCAL,
    DECL_GLOBAL,
    LOAD_LOCAL,
    LOAD_GLOBAL,
    STORE_LOCAL,
    STORE_GLOBAL,
    CLEAR_LOCALS,
    UNARY,
    BINARY,
    COMPARE,
    CHECK_BOOL,
    JUMP,
    JUMP_FALSE,
    JUMP_TRUE,
    PRECALL,
    CALL,
    RETURN,
    HALT,
    BUILD_ARRAY,
    CHECK_STRUCT,
    CHECK_FIELD,
    BUILD_STRUCT,
    GET_FIELD,
    GET_INDEX,
    GET_SLICE,
    SET_FIELD,
    SET_INDEX,
    ITER_BEGIN,
    ITER_NEXT,
    ITER_END
};
struct Instruction {
    Op op;
    std::uint32_t a{}, b{};
};
} // namespace myb
