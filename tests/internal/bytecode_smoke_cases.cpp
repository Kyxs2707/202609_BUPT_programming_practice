#include "bytecode/program.hpp"
#include "bytecode/verify.hpp"
#include "test_support.hpp"
using namespace myb;
MYB_TEST(scalar_bytecode_verify) {
    EmergencyFailure first;
    CodeUnit unit;
    unit.constants = {Value{std::int64_t{1}}, Value{std::int64_t{2}}};
    unit.instructions = {{Op::CONST, 0, 0},
                         {Op::CONST, 1, 0},
                         {Op::BINARY, static_cast<std::uint32_t>(BinaryOp::Add), 0},
                         {Op::DROP, 0, 0},
                         {Op::HALT, 0, 0}};
    unit.locations.resize(5);
    verify(unit, first);
    REQUIRE(unit.max_operand == 2);
    unit.instructions[0] = {Op::DROP, 0, 0};
    bool bad = false;
    try {
        verify(unit, first);
    } catch (const HostFailure &ex) {
        bad = ex.header().domain == FailureDomain::Internal;
    }
    REQUIRE(bad);
}
