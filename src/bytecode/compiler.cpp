#include "bytecode/compiler.hpp"
#include "ast/nodes.hpp"
#include "bytecode/verify.hpp"
#include "runtime/text_builder.hpp"
#include <limits>
#ifdef MYB_TEST_HOOKS
#include "test_hooks.hpp"
#endif
namespace myb {
namespace {
std::uint32_t index_of(std::size_t n, ErrorSite site) {
    if (n >= std::numeric_limits<std::uint32_t>::max())
        fail(site, FailureDomain::Resource, ReasonCode::Capacity, "bytecode table is too large");
    return static_cast<std::uint32_t>(n);
}
} // namespace
void BytecodeCompiler::compile(const CheckedAst &checked, CompiledProgram &out) {
    if (!out.units.empty())
        fail({first_, {}}, FailureDomain::Internal, ReasonCode::Invariant,
             "program already compiled");
    checked_ = &checked;
    program_ = &out;
    out.units.emplace_back();
    unit_ = &out.units.front();
    checked.program->accept(*this);
    emit(Op::HALT, checked.program->span());
    verify(*unit_, first_);
    checked_ = nullptr;
    unit_ = nullptr;
    program_ = nullptr;
}
void BytecodeCompiler::emit(Op op, SourceSpan span, std::uint32_t a) {
    (void)index_of(unit_->instructions.size(), {first_, span});
    unit_->instructions.push_back({op, a, 0});
    unit_->locations.push_back(span);
}
void BytecodeCompiler::constant(Value value, SourceSpan span) {
    auto index = index_of(unit_->constants.size(), {first_, span});
#ifdef MYB_TEST_HOOKS
    TestHooks::hit(FaultPoint::ProgramConstants);
#endif
    unit_->constants.push_back(value); // no MYB collection before container root takes over
    emit(Op::CONST, span, index);
}
void BytecodeCompiler::literal(const Node &node) {
    auto found = checked_->literals.find(&node);
    if (found == checked_->literals.end())
        fail({first_, node.span()}, FailureDomain::Internal, ReasonCode::Invariant,
             "missing normalized literal");
    std::visit(
        [&](const auto &value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::string>) {
                RootedValue result(roots_);
                TextBuilder builder(heap_);
                builder.append(std::span<const char>(value.data(), value.size()));
                heap_.make_string(std::move(builder), result);
                constant(result.get(), node.span());
            } else
                constant(Value{value}, node.span());
        },
        found->second);
}
void BytecodeCompiler::visit(const AstProgram &n) {
    for (const auto &item : n.items())
        item.accept(*this);
}
void BytecodeCompiler::visit(const IntExpr &n) {
    literal(n);
}
void BytecodeCompiler::visit(const FloatExpr &n) {
    literal(n);
}
void BytecodeCompiler::visit(const StringExpr &n) {
    literal(n);
}
void BytecodeCompiler::visit(const BoolExpr &n) {
    constant(Value{n.value()}, n.span());
}
void BytecodeCompiler::visit(const NullExpr &n) {
    constant(Value{NullValue{}}, n.span());
}
void BytecodeCompiler::visit(const NameExpr &n) {
    emit(Op::LOAD_GLOBAL, n.name_span(), program_->intern_name(n.name(), first_));
}
void BytecodeCompiler::visit(const GroupExpr &n) {
    n.expr().accept(*this);
}
void BytecodeCompiler::visit(const UnaryExpr &n) {
    if (checked_->literals.contains(&n)) {
        literal(n);
        return;
    }
    n.operand().accept(*this);
    emit(Op::UNARY, n.operator_span(), static_cast<std::uint32_t>(n.op()));
}
void BytecodeCompiler::visit(const BinaryExpr &n) {
    if (!std::holds_alternative<BinaryOp>(n.op()))
        unsupported(n);
    n.left().accept(*this);
    n.right().accept(*this);
    emit(Op::BINARY, n.operator_span(), static_cast<std::uint32_t>(std::get<BinaryOp>(n.op())));
}
void BytecodeCompiler::visit(const CallExpr &n) {
    auto index = index_of(unit_->calls.size(), {first_, n.span()});
    auto arity = index_of(n.args().size(), {first_, n.span()});
    unit_->calls.push_back({program_->intern_name(n.name(), first_), arity});
    emit(Op::PRECALL, n.span(), index);
    for (const auto &arg : n.args())
        arg.accept(*this);
    emit(Op::CALL, n.span(), index);
}
void BytecodeCompiler::visit(const ExprStmt &n) {
    n.expr().accept(*this);
    emit(Op::DROP, n.span());
}
[[noreturn]] void BytecodeCompiler::unsupported(const Node &n) {
    fail({first_, n.span()}, FailureDomain::Internal, ReasonCode::Invariant,
         "syntax is outside the M1 executable subset");
}
#define MYB_UNSUPPORTED(T)                                                                         \
    void BytecodeCompiler::visit(const T &n) {                                                     \
        unsupported(n);                                                                            \
    }
MYB_UNSUPPORTED(FunctionDecl)
MYB_UNSUPPORTED(StructDecl)
MYB_UNSUPPORTED(LogicalExpr)
MYB_UNSUPPORTED(ArrayExpr)
MYB_UNSUPPORTED(ConstructExpr)
MYB_UNSUPPORTED(FieldExpr)
MYB_UNSUPPORTED(IndexExpr)
MYB_UNSUPPORTED(SliceExpr)
MYB_UNSUPPORTED(LetStmt)
MYB_UNSUPPORTED(AssignStmt)
MYB_UNSUPPORTED(BlockStmt)
MYB_UNSUPPORTED(IfStmt)
MYB_UNSUPPORTED(WhileStmt)
MYB_UNSUPPORTED(ForStmt)
MYB_UNSUPPORTED(ReturnStmt)
MYB_UNSUPPORTED(BreakStmt)
MYB_UNSUPPORTED(ContinueStmt)
#undef MYB_UNSUPPORTED
} // namespace myb
