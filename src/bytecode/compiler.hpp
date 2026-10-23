#pragma once
#include "ast/visitor.hpp"
#include "bytecode/program.hpp"
#include "semantic/checked_ast.hpp"
namespace myb {
class BytecodeCompiler final : private AstVisitor {
  public:
    BytecodeCompiler(Heap &heap, RootRegistry &roots, EmergencyFailure &first)
        : heap_(heap), roots_(roots), first_(first) {}
    void compile(const CheckedAst &, CompiledProgram &);

  private:
    void emit(Op, SourceSpan, std::uint32_t a = 0);
    void constant(Value, SourceSpan);
    void literal(const Node &);
    [[noreturn]] void unsupported(const Node &);
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
    Heap &heap_;
    RootRegistry &roots_;
    EmergencyFailure &first_;
    const CheckedAst *checked_{};
    CompiledProgram *program_{};
    CodeUnit *unit_{};
};
} // namespace myb
