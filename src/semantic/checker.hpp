#pragma once
#include "diagnostic/failure.hpp"
#include "semantic/checked_ast.hpp"
namespace myb {
class Checker final : public AstVisitor {
  public:
    explicit Checker(EmergencyFailure &failure) : failure_(failure) {}
    CheckedAst check(const AstProgram &);
    void visit(const AstProgram &) override;
    void visit(const FunctionDecl &) override;
    void visit(const StructDecl &) override;
    void visit(const IntExpr &) override;
    void visit(const FloatExpr &) override;
    void visit(const StringExpr &) override;
    void visit(const BoolExpr &) override;
    void visit(const NullExpr &) override;
    void visit(const NameExpr &) override;
    void visit(const GroupExpr &) override;
    void visit(const UnaryExpr &) override;
    void visit(const BinaryExpr &) override;
    void visit(const LogicalExpr &) override;
    void visit(const CallExpr &) override;
    void visit(const ArrayExpr &) override;
    void visit(const ConstructExpr &) override;
    void visit(const FieldExpr &) override;
    void visit(const IndexExpr &) override;
    void visit(const SliceExpr &) override;
    void visit(const LetStmt &) override;
    void visit(const AssignStmt &) override;
    void visit(const ExprStmt &) override;
    void visit(const BlockStmt &) override;
    void visit(const IfStmt &) override;
    void visit(const WhileStmt &) override;
    void visit(const ForStmt &) override;
    void visit(const ReturnStmt &) override;
    void visit(const BreakStmt &) override;
    void visit(const ContinueStmt &) override;

  private:
    std::uint64_t magnitude(const IntExpr &);
    [[noreturn]] void unsupported(const Node &);
    EmergencyFailure &failure_;
    LiteralTable literals_;
};
} // namespace myb
