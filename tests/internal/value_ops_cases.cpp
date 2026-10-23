#include "runtime/value_ops.hpp"
#include "test_support.hpp"
#include <cmath>
#include <limits>
using namespace myb;

MYB_TEST(scalar_value_ops) {
    EmergencyFailure first;
    ErrorSite site{first, {}};
    auto i = [](std::int64_t n) { return Value{n}; };
    REQUIRE(std::get<std::int64_t>(ValueOps::binary(BinaryOp::Add, i(1), i(2), site).data) == 3);
    REQUIRE(std::get<double>(ValueOps::binary(BinaryOp::Divide, i(3), i(2), site).data) == 1.5);
    REQUIRE(std::get<std::int64_t>(
                ValueOps::binary(BinaryOp::FloorDivide, i(-7), i(3), site).data) == -3);
    REQUIRE(std::get<std::int64_t>(ValueOps::binary(BinaryOp::Modulo, i(-7), i(3), site).data) ==
            2);
    REQUIRE(std::get<std::int64_t>(ValueOps::binary(BinaryOp::Modulo, i(7), i(-3), site).data) ==
            -2);
    auto minimum = i(std::numeric_limits<std::int64_t>::min());
    REQUIRE(std::get<std::int64_t>(ValueOps::binary(BinaryOp::Modulo, minimum, i(-1), site).data) ==
            0);
    REQUIRE(std::signbit(std::get<double>(ValueOps::unary(UnaryOp::Minus, Value{0.0}, site).data)));
    struct Bad {
        BinaryOp op;
        Value lhs, rhs;
        ReasonCode reason;
    };
    Bad cases[] = {
        {BinaryOp::Add, i(std::numeric_limits<std::int64_t>::max()), i(1), ReasonCode::Arithmetic},
        {BinaryOp::Multiply, minimum, i(-1), ReasonCode::Arithmetic},
        {BinaryOp::FloorDivide, minimum, i(-1), ReasonCode::Arithmetic},
        {BinaryOp::Divide, i(1), Value{-0.0}, ReasonCode::Arithmetic},
        {BinaryOp::Add, Value{true}, i(1), ReasonCode::Type},
        {BinaryOp::Add, Value{std::numeric_limits<double>::max()},
         Value{std::numeric_limits<double>::max()}, ReasonCode::Arithmetic},
    };
    for (const auto &c : cases) {
        bool caught = false;
        try {
            (void)ValueOps::binary(c.op, c.lhs, c.rhs, site);
        } catch (const MybRuntimeFailure &f) {
            caught = f.header().reason == c.reason;
        }
        REQUIRE(caught);
    }
}
