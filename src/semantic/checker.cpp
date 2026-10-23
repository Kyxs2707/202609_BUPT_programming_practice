#include "semantic/checker.hpp"
#include "runtime/decimal_codec.hpp"
#include <limits>
namespace myb {
CheckedAst Checker::check(const AstProgram &program) {
    literals_.clear();
    program.accept(*this);
    return {&program, {}, std::move(literals_)};
}
void Checker::visit(const AstProgram &n) {
    for (const auto &item : n.items())
        item.accept(*this);
}
std::uint64_t Checker::magnitude(const IntExpr &n) {
    constexpr std::uint64_t limit = std::uint64_t{1} << 63;
    std::uint64_t value = 0;
    for (char c : n.digits()) {
        if (c < '0' || c > '9')
            fail({failure_, n.span()}, FailureDomain::Internal, ReasonCode::Invariant,
                 "invalid integer AST payload");
        auto digit = static_cast<unsigned>(c - '0');
        if (value > (limit - digit) / 10)
            fail({failure_, n.span()}, FailureDomain::Frontend, ReasonCode::Range,
                 "integer literal exceeds Int64 range");
        value = value * 10 + digit;
    }
    return value;
}
void Checker::visit(const IntExpr &n) {
    auto value = magnitude(n);
    if (value > static_cast<std::uint64_t>(INT64_MAX))
        fail({failure_, n.span()}, FailureDomain::Frontend, ReasonCode::Range,
             "positive integer literal exceeds Int64 range");
    literals_.emplace(&n, static_cast<std::int64_t>(value));
}
void Checker::visit(const FloatExpr &n) {
    auto p = DecimalCodec::parse_float(n.text(), ParseMode::SourceLiteral);
    if (!p.value)
        fail({failure_, n.span()}, FailureDomain::Frontend,
             p.error == NumericParseError::Range ? ReasonCode::Range : ReasonCode::Syntax,
             "invalid floating-point literal");
    literals_.emplace(&n, *p.value);
}
void Checker::visit(const StringExpr &n) {
    literals_.emplace(&n, std::string(n.value()));
}
void Checker::visit(const BoolExpr &) {}
void Checker::visit(const NullExpr &) {}
void Checker::visit(const NameExpr &) {}
void Checker::visit(const GroupExpr &n) {
    n.expr().accept(*this);
}
void Checker::visit(const UnaryExpr &n) {
    if (n.op() == UnaryOp::Minus) {
        // 只接纳负号的直接整数孩子；中间的 GroupExpr 会保留并走普通范围检查。
        if (auto *integer = dynamic_cast<const IntExpr *>(&n.operand());
            integer && magnitude(*integer) == (std::uint64_t{1} << 63)) {
            // 以负号节点记录 MIN，跳过不能单独表示为 Int64 的正幅值孩子。
            literals_.emplace(&n, INT64_MIN);
            return;
        }
    }
    n.operand().accept(*this);
}
void Checker::visit(const BinaryExpr &n) {
    n.left().accept(*this);
    n.right().accept(*this);
}
void Checker::visit(const CallExpr &n) {
    for (const auto &arg : n.args())
        arg.accept(*this);
}
void Checker::visit(const ExprStmt &n) {
    n.expr().accept(*this);
}
[[noreturn]] void Checker::unsupported(const Node &n) {
    fail({failure_, n.span()}, FailureDomain::Frontend, ReasonCode::Syntax,
         "syntax is unavailable in the M1 subset");
}
#define MYB_UNSUPPORTED(T)                                                                         \
    void Checker::visit(const T &n) {                                                              \
        unsupported(n);                                                                            \
    }
MYB_UNSUPPORTED(FunctionDecl)
MYB_UNSUPPORTED(StructDecl) MYB_UNSUPPORTED(LogicalExpr) MYB_UNSUPPORTED(ArrayExpr)
    MYB_UNSUPPORTED(ConstructExpr) MYB_UNSUPPORTED(FieldExpr) MYB_UNSUPPORTED(IndexExpr)
        MYB_UNSUPPORTED(SliceExpr) MYB_UNSUPPORTED(LetStmt) MYB_UNSUPPORTED(AssignStmt)
            MYB_UNSUPPORTED(BlockStmt) MYB_UNSUPPORTED(IfStmt) MYB_UNSUPPORTED(WhileStmt)
                MYB_UNSUPPORTED(ForStmt) MYB_UNSUPPORTED(ReturnStmt) MYB_UNSUPPORTED(BreakStmt)
                    MYB_UNSUPPORTED(ContinueStmt)
#undef MYB_UNSUPPORTED
} // namespace myb
