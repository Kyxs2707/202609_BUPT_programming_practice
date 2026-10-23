#pragma once
#include "runtime/operators.hpp"
#include <cstdint>
namespace myb {
/**
 * @brief 标识字节码操作；编译器、核验器和 VM 当前仅实现 M1 的线性表达式子集。
 *
 * 枚举同时保留后续局部变量、控制流和容器操作；存在枚举项不表示当前可执行。
 */
enum class Op : std::uint16_t {
    CONST, ///< 将当前 CodeUnit 的常量压入操作数区。
    DUP,
    DROP, ///< 清空并移除栈顶操作数，使其退出 VM 根扫描范围。
    DECL_LOCAL,
    DECL_GLOBAL,
    LOAD_LOCAL,
    LOAD_GLOBAL, ///< 查询全局名称；M1 尚无变量绑定，执行时报告未声明名称。
    STORE_LOCAL,
    STORE_GLOBAL,
    CLEAR_LOCALS,
    UNARY, ///< 将栈顶值替换为一元运算结果。
    BINARY, ///< 将栈顶两个值替换为二元运算结果。
    COMPARE,
    CHECK_BOOL,
    JUMP,
    JUMP_FALSE,
    JUMP_TRUE,
    PRECALL, ///< 按当前 CodeUnit 的调用点预检内置名称和参数数量。
    CALL, ///< 消费调用点指定数量的实参，并压入内置回调的结果。
    RETURN,
    HALT, ///< 在空操作数栈上结束顶层执行。
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
/**
 * @brief 保存操作码及其两个无符号操作数。
 *
 * a 的含义由操作码决定；M1 指令的 b 必须为零，核验器会拒绝非零值。
 */
struct Instruction {
    Op op;
    std::uint32_t a{}, b{};
};
} // namespace myb
