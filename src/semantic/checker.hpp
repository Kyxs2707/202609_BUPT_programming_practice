#pragma once
#include "diagnostic/failure.hpp"
#include "semantic/checked_ast.hpp"
namespace myb {
/**
 * @brief 在执行前遍历整个 M1 程序，并生成规范化字面量侧表。
 *
 * 当前接收字面量、名称、分组、一元/二元节点、命名调用及表达式语句；
 * 其余预留节点由对应 visit 重载拒绝。此阶段不求运行值，
 * 名称读取和调用目标的合法性留至运行时判断。
 * 各 visit 通过对象成员累计结果；调用方通过 check() 开始完整检查。
 * 借用的 EmergencyFailure 须比检查器存活更久。
 */
class Checker final : public AstVisitor {
  public:
    explicit Checker(EmergencyFailure &failure) : failure_(failure) {}
    /**
     * @brief 清除上次临时结果，检查整棵树并交付本次侧表。
     * @return 借用输入程序、独占侧表的结果；调用方负责输入树的寿命。
     * @throws FrontendFailure 发现超范围字面量或 M1 未支持的节点。
     * @throws HostFailure AST 整数载荷违反内部约束。
     * @throws std::bad_alloc 侧表或宿主字符串分配失败。
     * 任一异常均终止本次检查，不交付部分结果。
     */
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
    /**
     * @brief 按十进制逐位读取整数幅值，并在乘加前检查上限。
     * @return 0～2^63 的无符号幅值，保留最小负整数所需的额外一个正幅值。
     * @throws FrontendFailure 幅值超过 2^63。
     * @throws HostFailure 词素含非十进制数字，属于 AST 内部约束失败。
     */
    std::uint64_t magnitude(const IntExpr &);
    /**
     * @brief 在节点源码范围报告当前 M1 子集尚未支持的结构。
     * @throws FrontendFailure 故障类别为 Frontend/Syntax。
     */
    [[noreturn]] void unsupported(const Node &);
    EmergencyFailure &failure_;
    LiteralTable literals_;
};
} // namespace myb
