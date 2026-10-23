#include "vm/vm.hpp"
#include "runtime/value_ops.hpp"
#include <algorithm>
#include <limits>
#ifdef MYB_TEST_HOOKS
#include "test_hooks.hpp"
#endif
namespace myb {
VM::VM(Heap &heap, RootRegistry &roots, HostIO &io, EmergencyFailure &first,
       const BuiltinRegistry &builtins) noexcept
    : heap_(heap), roots_(roots), io_(io), first_(first), builtins_(builtins),
      root_set_(roots, this, trace_values) {}
void VM::trace_values(void *self, Marker &marker) {
    auto &vm = *static_cast<VM *>(self);
    // 容量和旧槽值不能决定保活；仅活动帧中的绑定状态与有效栈深决定扫描范围。
    for (const auto &frame : vm.frames_) {
        for (auto i = frame.base; i < frame.operand_base; ++i)
            if (vm.states_[i] == BindingState::Initialized)
                marker.mark(vm.values_[i]);
        for (std::size_t i = 0; i < frame.sp; ++i)
            marker.mark(vm.values_[frame.operand_base + i]);
    }
}
[[noreturn]] void VM::invalid() {
    fail({first_, {}}, FailureDomain::Internal, ReasonCode::Invariant, "invalid VM state");
}
void VM::push(Value value) {
    auto &frame = frames_.back();
    if (frame.sp >= frame.region_end - frame.operand_base)
        invalid();
    values_[frame.operand_base + frame.sp] = value;
    ++frame.sp;
}
const CallSite &VM::call_site(const CodeUnit &unit, std::uint32_t index) {
    if (index >= unit.calls.size())
        invalid();
    return unit.calls[index];
}
void VM::execute(const CompiledProgram &program) {
    if (!frames_.empty() || program.top_unit >= program.units.size())
        invalid();
    const auto &unit = program.units[program.top_unit];
    const std::size_t locals = unit.local_count, stack = unit.max_operand;
    if (stack > std::numeric_limits<std::size_t>::max() - locals)
        invalid();
    const auto region = locals + stack;
    if (region > values_.max_size() || region > states_.max_size())
        fail({first_, {}}, FailureDomain::Resource, ReasonCode::Capacity,
             "VM value region is too large");
    // 在发布活动帧前完成宿主存储准备；调度和内置调用都不再增长这些容器。
    auto required = std::max<std::size_t>(64, region);
#ifdef MYB_TEST_HOOKS
    TestHooks::hit(FaultPoint::VmValues);
#endif
    values_.resize(required);
#ifdef MYB_TEST_HOOKS
    TestHooks::hit(FaultPoint::VmStates);
#endif
    states_.resize(required, BindingState::Dormant);
#ifdef MYB_TEST_HOOKS
    TestHooks::hit(FaultPoint::VmFrames);
#endif
    frames_.reserve(16);
    Frame frame;
    frame.unit_id = program.top_unit;
    frame.operand_base = locals;
    frame.region_end = region;
    frames_.push_back(frame);
    try {
        for (;;) {
            auto &current = frames_.back();
            if (current.next_ip >= unit.instructions.size())
                invalid();
            current.executing_ip = current.next_ip++;
            const auto ins = unit.instructions[current.executing_ip];
            const auto span = unit.locations[current.executing_ip];
            const ErrorSite site{first_, span};
            switch (ins.op) {
            case Op::CONST:
                if (ins.a >= unit.constants.size())
                    invalid();
                push(unit.constants[ins.a]);
                break;
            case Op::LOAD_GLOBAL:
                fail(site, FailureDomain::Runtime, ReasonCode::Name, "undeclared variable");
            case Op::UNARY: {
                if (current.sp < 1)
                    invalid();
                const auto index = current.operand_base + current.sp - 1;
                values_[index] = ValueOps::unary(static_cast<UnaryOp>(ins.a), values_[index], site);
                break;
            }
            case Op::BINARY: {
                if (current.sp < 2)
                    invalid();
                const auto lhs = current.operand_base + current.sp - 2;
                const auto result = ValueOps::binary(static_cast<BinaryOp>(ins.a), values_[lhs],
                                                     values_[lhs + 1], site);
                values_[lhs] = result;
                values_[lhs + 1] = Value{NullValue{}};
                --current.sp;
                break;
            }
            case Op::PRECALL: {
                const auto &call = call_site(unit, ins.a);
                if (call.target >= program.names.size())
                    invalid();
                (void)builtins_.precheck(program.names[call.target], call.arity, site);
                break;
            }
            case Op::CALL: {
                const auto &call = call_site(unit, ins.a);
                if (call.target >= program.names.size() || current.sp < call.arity)
                    invalid();
                const auto &target =
                    builtins_.precheck(program.names[call.target], call.arity, site);
                if (!target.invoke)
                    invalid();
                const auto base = current.operand_base + current.sp - call.arity;
                const auto saved_sp = current.sp - call.arity;
                RootedValue result(roots_);
                NativeContext context{heap_, roots_, io_, first_, span};
                // 回调借用连续实参区期间不增长值区或帧表，也不重入本 VM。
                // 保留原 sp，使回调中的 Heap 分配和 GC 仍能扫描全部实参。
                target.invoke(context, std::span<const Value>(values_.data() + base, call.arity),
                              result);
                // 先清旧实参再发布结果，零参数调用也使用预留结果槽；交接不分配或回收。
                for (std::size_t i = 0; i < call.arity; ++i)
                    values_[base + i] = Value{NullValue{}};
                current.sp = saved_sp;
                push(result.get()); // 结果成为有效操作数后，才允许 result 撤销临时根。
                break;
            }
            case Op::DROP:
                if (!current.sp)
                    invalid();
                values_[current.operand_base + --current.sp] = Value{NullValue{}};
                break;
            case Op::HALT:
                if (current.sp)
                    invalid();
                clear();
                return;
            default:
                invalid();
            }
        }
    } catch (const Failure &failure) {
        // 平台错误可能没有 MYB 位置；先补充当前指令位置，再清除用于定位的帧。
        auto header = failure.header();
        if (!header.span && !frames_.empty())
            header.span = unit.locations[frames_.back().executing_ip];
        clear();
        // 重建异常的宿主文本可能分配；固定首错记录仍保留原有失败类别。
        if (header.domain == FailureDomain::Runtime)
            throw MybRuntimeFailure(header, failure.what());
        throw HostFailure(header, failure.what());
    } catch (...) {
        clear();
        throw;
    }
}
void VM::clear() noexcept {
    for (auto &value : values_)
        value = Value{NullValue{}};
    for (auto &state : states_)
        state = BindingState::Dormant;
    frames_.clear();
}
} // namespace myb
