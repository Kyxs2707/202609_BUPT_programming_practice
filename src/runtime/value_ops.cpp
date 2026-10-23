#include "runtime/value_ops.hpp"
#include <cmath>
#include <limits>

namespace myb {
namespace {
[[noreturn]] void type_error(ErrorSite site) {
    fail(site, FailureDomain::Runtime, ReasonCode::Type, "numeric operands required");
}
[[noreturn]] void arithmetic_error(ErrorSite site) {
    fail(site, FailureDomain::Runtime, ReasonCode::Arithmetic,
         "division by zero or numeric overflow");
}
bool numeric(Value v) noexcept {
    return std::holds_alternative<std::int64_t>(v.data) || std::holds_alternative<double>(v.data);
}
/**
 * @brief 将已验证的数值提升为 Float64，允许 Int64 转换产生规定的舍入。
 * @pre v 仅含 Int64 或 double；类型检查由运算入口完成。
 * @return 浮点值表示，不用于混合数值比较。
 */
double as_float(Value v) noexcept {
    if (auto p = std::get_if<double>(&v.data))
        return *p;
    return static_cast<double>(std::get<std::int64_t>(v.data));
}
Value checked_float(double n, ErrorSite site) {
    if (!std::isfinite(n))
        arithmetic_error(site);
    return Value{n};
}
} // namespace
Value ValueOps::unary(UnaryOp op, Value value, ErrorSite site) {
    if (op == UnaryOp::Not) {
        auto p = std::get_if<bool>(&value.data);
        if (!p)
            fail(site, FailureDomain::Runtime, ReasonCode::Type, "Bool operand required");
        return Value{!*p};
    }
    if (!numeric(value))
        type_error(site);
    if (op == UnaryOp::Plus)
        return value;
    if (op != UnaryOp::Minus)
        fail(site, FailureDomain::Internal, ReasonCode::Invariant, "unknown unary operator");
    if (auto p = std::get_if<std::int64_t>(&value.data)) {
        if (*p == std::numeric_limits<std::int64_t>::min())
            arithmetic_error(site);
        return Value{-*p};
    }
    return Value{-std::get<double>(value.data)};
}
Value ValueOps::binary(BinaryOp op, Value lhs, Value rhs, ErrorSite site) {
    if (!numeric(lhs) || !numeric(rhs))
        type_error(site);
    auto a = std::get_if<std::int64_t>(&lhs.data);
    auto b = std::get_if<std::int64_t>(&rhs.data);
    if (op == BinaryOp::FloorDivide || op == BinaryOp::Modulo) {
        if (!a || !b)
            type_error(site);
        if (*b == 0)
            arithmetic_error(site);
        // MIN 与 -1 的宿主除法和余数均有未定义行为，先分别给出语言规定的结果。
        if (*a == std::numeric_limits<std::int64_t>::min() && *b == -1) {
            if (op == BinaryOp::Modulo)
                return Value{std::int64_t{0}};
            arithmetic_error(site);
        }
        auto q = *a / *b;
        auto r = *a % *b;
        // 把宿主向零截断修正为向下取整；直接修正余数，避免 q*b 的中间乘积溢出。
        if (r != 0 && ((r < 0) != (*b < 0))) {
            --q;
            r += *b;
        }
        return Value{op == BinaryOp::FloorDivide ? q : r};
    }
    if (a && b && op != BinaryOp::Divide) {
        std::int64_t n = 0;
        bool overflow = false;
        // 使用带溢出检测的宿主运算，先检查再交付 Int64，不执行有符号溢出表达式。
        switch (op) {
        case BinaryOp::Add:
            overflow = __builtin_add_overflow(*a, *b, &n);
            break;
        case BinaryOp::Subtract:
            overflow = __builtin_sub_overflow(*a, *b, &n);
            break;
        case BinaryOp::Multiply:
            overflow = __builtin_mul_overflow(*a, *b, &n);
            break;
        default:
            fail(site, FailureDomain::Internal, ReasonCode::Invariant, "unknown binary operator");
        }
        if (overflow)
            arithmetic_error(site);
        return Value{n};
    }
    const double x = as_float(lhs), y = as_float(rhs);
    switch (op) {
    case BinaryOp::Add:
        return checked_float(x + y, site);
    case BinaryOp::Subtract:
        return checked_float(x - y, site);
    case BinaryOp::Multiply:
        return checked_float(x * y, site);
    case BinaryOp::Divide:
        if (y == 0.0)
            arithmetic_error(site);
        return checked_float(x / y, site);
    default:
        fail(site, FailureDomain::Internal, ReasonCode::Invariant, "unknown binary operator");
    }
}
} // namespace myb
