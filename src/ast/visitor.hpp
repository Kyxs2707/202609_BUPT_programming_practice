#pragma once
namespace myb {
class AstProgram;
class FunctionDecl;
class StructDecl;
class IntExpr;
class FloatExpr;
class StringExpr;
class BoolExpr;
class NullExpr;
class NameExpr;
class GroupExpr;
class UnaryExpr;
class BinaryExpr;
class LogicalExpr;
class CallExpr;
class ArrayExpr;
class ConstructExpr;
class FieldExpr;
class IndexExpr;
class SliceExpr;
class LetStmt;
class AssignStmt;
class ExprStmt;
class BlockStmt;
class IfStmt;
class WhileStmt;
class ForStmt;
class ReturnStmt;
class BreakStmt;
class ContinueStmt;
/**
 * @brief 为每种具体 AST 节点提供只读访问入口。
 *
 * visit 的参数借用被访问树，仅在所属树存活期间有效。各重载自行选择
 * 子节点访问顺序并保存阶段状态；accept 仅作分派。新增节点时，
 * 检查器和生成器须同时实现对应重载，不能用默认空实现跳过检查。
 */
class AstVisitor {
  public:
    virtual ~AstVisitor() noexcept = default;
    virtual void visit(const AstProgram &) = 0;
    virtual void visit(const FunctionDecl &) = 0;
    virtual void visit(const StructDecl &) = 0;
    virtual void visit(const IntExpr &) = 0;
    virtual void visit(const FloatExpr &) = 0;
    virtual void visit(const StringExpr &) = 0;
    virtual void visit(const BoolExpr &) = 0;
    virtual void visit(const NullExpr &) = 0;
    virtual void visit(const NameExpr &) = 0;
    virtual void visit(const GroupExpr &) = 0;
    virtual void visit(const UnaryExpr &) = 0;
    virtual void visit(const BinaryExpr &) = 0;
    virtual void visit(const LogicalExpr &) = 0;
    virtual void visit(const CallExpr &) = 0;
    virtual void visit(const ArrayExpr &) = 0;
    virtual void visit(const ConstructExpr &) = 0;
    virtual void visit(const FieldExpr &) = 0;
    virtual void visit(const IndexExpr &) = 0;
    virtual void visit(const SliceExpr &) = 0;
    virtual void visit(const LetStmt &) = 0;
    virtual void visit(const AssignStmt &) = 0;
    virtual void visit(const ExprStmt &) = 0;
    virtual void visit(const BlockStmt &) = 0;
    virtual void visit(const IfStmt &) = 0;
    virtual void visit(const WhileStmt &) = 0;
    virtual void visit(const ForStmt &) = 0;
    virtual void visit(const ReturnStmt &) = 0;
    virtual void visit(const BreakStmt &) = 0;
    virtual void visit(const ContinueStmt &) = 0;
};
} // namespace myb
