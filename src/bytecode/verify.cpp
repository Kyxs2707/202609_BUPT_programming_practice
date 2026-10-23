#include "bytecode/verify.hpp"
#include <algorithm>
#include <limits>
namespace myb {
void verify(CodeUnit &unit, EmergencyFailure &first) {
    const auto bad = [&] {
        fail({first, {}}, FailureDomain::Internal, ReasonCode::Invariant, "invalid M1 bytecode");
    };
    if (unit.instructions.empty() || unit.instructions.size() != unit.locations.size())
        bad();
    // M1 没有可执行跳转，按唯一线性路径计算栈深，不展开内置回调。
    std::uint64_t depth = 0, maximum = 0;
    bool halted = false;
    for (auto ins : unit.instructions) {
        if (halted || ins.b != 0)
            bad();
        std::uint64_t consumed = 0, produced = 0;
        switch (ins.op) {
        case Op::CONST:
            if (ins.a >= unit.constants.size())
                bad();
            produced = 1;
            break;
        case Op::LOAD_GLOBAL:
            produced = 1;
            break;
        case Op::UNARY:
            if (ins.a > static_cast<std::uint32_t>(UnaryOp::Not))
                bad();
            consumed = produced = 1;
            break;
        case Op::BINARY:
            if (ins.a > static_cast<std::uint32_t>(BinaryOp::Modulo))
                bad();
            consumed = 2;
            produced = 1;
            break;
        case Op::PRECALL:
            if (ins.a >= unit.calls.size())
                bad();
            break;
        case Op::CALL:
            if (ins.a >= unit.calls.size())
                bad();
            consumed = unit.calls[ins.a].arity;
            produced = 1;
            break;
        case Op::DROP:
            if (ins.a != 0)
                bad();
            consumed = 1;
            break;
        case Op::HALT:
            if (depth != 0 || ins.a != 0)
                bad();
            halted = true;
            break;
        default:
            bad();
        }
        if (depth < consumed)
            bad();
        depth = depth - consumed + produced;
        maximum = std::max(maximum, depth);
        if (maximum > std::numeric_limits<std::uint32_t>::max())
            fail({first, {}}, FailureDomain::Resource, ReasonCode::Capacity,
                 "operand stack is too large");
    }
    if (!halted)
        bad();
    unit.max_operand = static_cast<std::uint32_t>(maximum);
}
} // namespace myb
