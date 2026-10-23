#pragma once
#include "ast/visitor.hpp"
#include "bytecode/program.hpp"
#include "semantic/checked_ast.hpp"
namespace myb {
/**
 * @brief 将已检查 AST 生成为 M1 字节码，并核验操作数容量。
 *
 * 当前支持常量、未绑定名称读取、一元和算术二元表达式、命名内置调用及表达式语句。
 * 其他语法进入 unsupported；编译器借用的 Heap、RootRegistry 和首错记录须覆盖其使用期。
 */
class BytecodeCompiler final : private AstVisitor {
  public:
    /** @brief 借用同一会话的堆、根登记表和首错记录，不取得其所有权。 */
    BytecodeCompiler(Heap &heap, RootRegistry &roots, EmergencyFailure &first)
        : heap_(heap), roots_(roots), first_(first) {}
    /**
     * @brief 生成顶层单元，添加 HALT 并计算 max_operand。
     * @param[in] checked 编译期间借用的已检查 AST 及规范化字面量。
     * @param[in,out] out 接收独立拥有的代码表、名称和常量句柄。
     * @pre out.units 为空，且 out 的常量根登记于本编译器使用的 RootRegistry。
     * @throws Failure 输入含 M1 未实现语法、字面量记录缺失、编号超限或产物核验失败。
     *
     * 字符串常量创建可触发 GC，先由临时根接收，再交给 out 的常量根集合。
     * 失败时 out 可含部分产物，由调用者随会话清理；成功产物不借用 checked。
     */
    void compile(const CheckedAst &, CompiledProgram &);

  private:
    /** @brief 追加指令及对应源码范围，检查指令数量且将 b 置零。 */
    void emit(Op, SourceSpan, std::uint32_t a = 0);
    /**
     * @brief 将值加入程序常量根集合，并生成引用该常量的 CONST。
     * @pre 受管 value 在常量表接管前由调用者保活；本操作不触发 MYB GC。
     *
     * 宿主容器增长仍可能抛出异常，调用者的临时根须覆盖整个调用。
     */
    void constant(Value, SourceSpan);
    /** @brief 读取规范化字面量，字符串经临时根创建后再写入常量表。 */
    void literal(const Node &);
    /** @brief 以内部实现范围错误终止不属于 M1 可执行子集的节点编译。 */
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
