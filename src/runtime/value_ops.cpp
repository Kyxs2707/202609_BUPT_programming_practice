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
        if (*a == std::numeric_limits<std::int64_t>::min() && *b == -1) {
            if (op == BinaryOp::Modulo)
                return Value{std::int64_t{0}};
            arithmetic_error(site);
        }
        auto q = *a / *b;
        auto r = *a % *b;
        if (r != 0 && ((r < 0) != (*b < 0))) {
            --q;
            r += *b;
        }
        return Value{op == BinaryOp::FloorDivide ? q : r};
    }
    if (a && b && op != BinaryOp::Divide) {
        std::int64_t n = 0;
        bool overflow = false;
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
