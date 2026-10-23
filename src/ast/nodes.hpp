#pragma once
#include "ast/node.hpp"
#include "ast/visitor.hpp"
#include "runtime/operators.hpp"
#include <span>
#include <string>
#include <string_view>
#include <variant>
namespace myb {
using ArithmeticOrCompare = std::variant<BinaryOp, CompareOp>;
struct TypeRef {
    std::string name;
    bool nullable;
    SourceSpan span;
};
struct Parameter {
    std::string name;
    SourceSpan name_span;
};
struct FieldDecl {
    std::string name;
    SourceSpan name_span;
    TypeRef type;
};
class FieldInit {
  public:
    FieldInit(std::string n, SourceSpan s, std::unique_ptr<Expr> v)
        : name_(std::move(n)), span_(s), value_(std::move(v)) {}
    std::string_view name() const {
        return name_;
    }
    SourceSpan name_span() const {
        return span_;
    }
    const Expr &value() const {
        return *value_;
    }

  private:
    std::string name_;
    SourceSpan span_;
    std::unique_ptr<Expr> value_;
};
class AstProgram final : public Node {
  public:
    AstProgram(SourceSpan, std::uint16_t, std::vector<std::unique_ptr<TopItem>> items);
    ~AstProgram() noexcept override;
    void accept(AstVisitor &) const override;
    const ReadOnlyNodes<TopItem> &items() const noexcept {
        return items_;
    }

  private:
    ReadOnlyNodes<TopItem> items_;
};
class FunctionDecl final : public TopItem {
  public:
    FunctionDecl(SourceSpan, std::uint16_t, std::string name, SourceSpan name_span,
                 std::vector<Parameter> parameters, std::unique_ptr<BlockStmt> body);
    ~FunctionDecl() noexcept override;
    void accept(AstVisitor &) const override;
    std::string_view name() const noexcept {
        return name_;
    }
    SourceSpan name_span() const noexcept {
        return name_span_;
    }
    std::span<const Parameter> parameters() const noexcept {
        return parameters_;
    }
    const BlockStmt &body() const noexcept {
        return *body_;
    }

  private:
    std::string name_;
    SourceSpan name_span_;
    std::vector<Parameter> parameters_;
    std::unique_ptr<BlockStmt> body_;
};
class StructDecl final : public TopItem {
  public:
    StructDecl(SourceSpan, std::uint16_t, std::string name, SourceSpan name_span,
               std::vector<FieldDecl> fields);
    ~StructDecl() noexcept override;
    void accept(AstVisitor &) const override;
    std::string_view name() const noexcept {
        return name_;
    }
    SourceSpan name_span() const noexcept {
        return name_span_;
    }
    std::span<const FieldDecl> fields() const noexcept {
        return fields_;
    }

  private:
    std::string name_;
    SourceSpan name_span_;
    std::vector<FieldDecl> fields_;
};
class IntExpr final : public Expr {
  public:
    IntExpr(SourceSpan, std::uint16_t, std::string digits);
    ~IntExpr() noexcept override;
    void accept(AstVisitor &) const override;
    std::string_view digits() const noexcept {
        return digits_;
    }

  private:
    std::string digits_;
};
class FloatExpr final : public Expr {
  public:
    FloatExpr(SourceSpan, std::uint16_t, std::string text);
    ~FloatExpr() noexcept override;
    void accept(AstVisitor &) const override;
    std::string_view text() const noexcept {
        return text_;
    }

  private:
    std::string text_;
};
class StringExpr final : public Expr {
  public:
    StringExpr(SourceSpan, std::uint16_t, std::string value);
    ~StringExpr() noexcept override;
    void accept(AstVisitor &) const override;
    std::string_view value() const noexcept {
        return value_;
    }

  private:
    std::string value_;
};
class BoolExpr final : public Expr {
  public:
    BoolExpr(SourceSpan, std::uint16_t, bool value);
    ~BoolExpr() noexcept override;
    void accept(AstVisitor &) const override;
    bool value() const noexcept {
        return value_;
    }

  private:
    bool value_;
};
class NullExpr final : public Expr {
  public:
    NullExpr(SourceSpan, std::uint16_t);
    ~NullExpr() noexcept override;
    void accept(AstVisitor &) const override;

  private:
};
class NameExpr final : public Expr {
  public:
    NameExpr(SourceSpan, std::uint16_t, std::string name, SourceSpan name_span);
    ~NameExpr() noexcept override;
    void accept(AstVisitor &) const override;
    std::string_view name() const noexcept {
        return name_;
    }
    SourceSpan name_span() const noexcept {
        return name_span_;
    }

  private:
    std::string name_;
    SourceSpan name_span_;
};
class GroupExpr final : public Expr {
  public:
    GroupExpr(SourceSpan, std::uint16_t, std::unique_ptr<Expr> expr);
    ~GroupExpr() noexcept override;
    void accept(AstVisitor &) const override;
    const Expr &expr() const noexcept {
        return *expr_;
    }

  private:
    std::unique_ptr<Expr> expr_;
};
class UnaryExpr final : public Expr {
  public:
    UnaryExpr(SourceSpan, std::uint16_t, UnaryOp op, SourceSpan operator_span,
              std::unique_ptr<Expr> operand);
    ~UnaryExpr() noexcept override;
    void accept(AstVisitor &) const override;
    UnaryOp op() const noexcept {
        return op_;
    }
    SourceSpan operator_span() const noexcept {
        return operator_span_;
    }
    const Expr &operand() const noexcept {
        return *operand_;
    }

  private:
    UnaryOp op_;
    SourceSpan operator_span_;
    std::unique_ptr<Expr> operand_;
};
class BinaryExpr final : public Expr {
  public:
    BinaryExpr(SourceSpan, std::uint16_t, ArithmeticOrCompare op, SourceSpan operator_span,
               std::unique_ptr<Expr> left, std::unique_ptr<Expr> right);
    ~BinaryExpr() noexcept override;
    void accept(AstVisitor &) const override;
    ArithmeticOrCompare op() const noexcept {
        return op_;
    }
    SourceSpan operator_span() const noexcept {
        return operator_span_;
    }
    const Expr &left() const noexcept {
        return *left_;
    }
    const Expr &right() const noexcept {
        return *right_;
    }

  private:
    ArithmeticOrCompare op_;
    SourceSpan operator_span_;
    std::unique_ptr<Expr> left_;
    std::unique_ptr<Expr> right_;
};
class LogicalExpr final : public Expr {
  public:
    LogicalExpr(SourceSpan, std::uint16_t, LogicalOp op, SourceSpan operator_span,
                std::unique_ptr<Expr> left, std::unique_ptr<Expr> right);
    ~LogicalExpr() noexcept override;
    void accept(AstVisitor &) const override;
    LogicalOp op() const noexcept {
        return op_;
    }
    SourceSpan operator_span() const noexcept {
        return operator_span_;
    }
    const Expr &left() const noexcept {
        return *left_;
    }
    const Expr &right() const noexcept {
        return *right_;
    }

  private:
    LogicalOp op_;
    SourceSpan operator_span_;
    std::unique_ptr<Expr> left_;
    std::unique_ptr<Expr> right_;
};
class CallExpr final : public Expr {
  public:
    CallExpr(SourceSpan, std::uint16_t, std::string name, SourceSpan name_span,
             std::vector<std::unique_ptr<Expr>> args);
    ~CallExpr() noexcept override;
    void accept(AstVisitor &) const override;
    std::string_view name() const noexcept {
        return name_;
    }
    SourceSpan name_span() const noexcept {
        return name_span_;
    }
    const ReadOnlyNodes<Expr> &args() const noexcept {
        return args_;
    }

  private:
    std::string name_;
    SourceSpan name_span_;
    ReadOnlyNodes<Expr> args_;
};
class ArrayExpr final : public Expr {
  public:
    ArrayExpr(SourceSpan, std::uint16_t, std::vector<std::unique_ptr<Expr>> elements);
    ~ArrayExpr() noexcept override;
    void accept(AstVisitor &) const override;
    const ReadOnlyNodes<Expr> &elements() const noexcept {
        return elements_;
    }

  private:
    ReadOnlyNodes<Expr> elements_;
};
class ConstructExpr final : public Expr {
  public:
    ConstructExpr(SourceSpan, std::uint16_t, std::string name, SourceSpan name_span,
                  std::vector<FieldInit> fields);
    ~ConstructExpr() noexcept override;
    void accept(AstVisitor &) const override;
    std::string_view name() const noexcept {
        return name_;
    }
    SourceSpan name_span() const noexcept {
        return name_span_;
    }
    std::span<const FieldInit> fields() const noexcept {
        return fields_;
    }

  private:
    std::string name_;
    SourceSpan name_span_;
    std::vector<FieldInit> fields_;
};
class FieldExpr final : public Expr {
  public:
    FieldExpr(SourceSpan, std::uint16_t, std::unique_ptr<Expr> receiver, std::string name,
              SourceSpan name_span, SourceSpan access_span);
    ~FieldExpr() noexcept override;
    void accept(AstVisitor &) const override;
    const Expr &receiver() const noexcept {
        return *receiver_;
    }
    std::string_view name() const noexcept {
        return name_;
    }
    SourceSpan name_span() const noexcept {
        return name_span_;
    }
    SourceSpan access_span() const noexcept {
        return access_span_;
    }

  private:
    std::unique_ptr<Expr> receiver_;
    std::string name_;
    SourceSpan name_span_;
    SourceSpan access_span_;
};
class IndexExpr final : public Expr {
  public:
    IndexExpr(SourceSpan, std::uint16_t, std::unique_ptr<Expr> receiver,
              std::unique_ptr<Expr> index, SourceSpan access_span);
    ~IndexExpr() noexcept override;
    void accept(AstVisitor &) const override;
    const Expr &receiver() const noexcept {
        return *receiver_;
    }
    const Expr &index() const noexcept {
        return *index_;
    }
    SourceSpan access_span() const noexcept {
        return access_span_;
    }

  private:
    std::unique_ptr<Expr> receiver_;
    std::unique_ptr<Expr> index_;
    SourceSpan access_span_;
};
class SliceExpr final : public Expr {
  public:
    SliceExpr(SourceSpan, std::uint16_t, std::unique_ptr<Expr> receiver,
              std::unique_ptr<Expr> start, std::unique_ptr<Expr> end, SourceSpan access_span);
    ~SliceExpr() noexcept override;
    void accept(AstVisitor &) const override;
    const Expr &receiver() const noexcept {
        return *receiver_;
    }
    const Expr *start() const noexcept {
        return start_.get();
    }
    const Expr *end() const noexcept {
        return end_.get();
    }
    SourceSpan access_span() const noexcept {
        return access_span_;
    }

  private:
    std::unique_ptr<Expr> receiver_;
    std::unique_ptr<Expr> start_;
    std::unique_ptr<Expr> end_;
    SourceSpan access_span_;
};
class LetStmt final : public Stmt {
  public:
    LetStmt(SourceSpan, std::uint16_t, std::string name, SourceSpan name_span,
            std::unique_ptr<Expr> initializer);
    ~LetStmt() noexcept override;
    void accept(AstVisitor &) const override;
    std::string_view name() const noexcept {
        return name_;
    }
    SourceSpan name_span() const noexcept {
        return name_span_;
    }
    const Expr *initializer() const noexcept {
        return initializer_.get();
    }

  private:
    std::string name_;
    SourceSpan name_span_;
    std::unique_ptr<Expr> initializer_;
};
class AssignStmt final : public Stmt {
  public:
    AssignStmt(SourceSpan, std::uint16_t, std::unique_ptr<Expr> target, std::unique_ptr<Expr> value,
               SourceSpan operator_span);
    ~AssignStmt() noexcept override;
    void accept(AstVisitor &) const override;
    const Expr &target() const noexcept {
        return *target_;
    }
    const Expr &value() const noexcept {
        return *value_;
    }
    SourceSpan operator_span() const noexcept {
        return operator_span_;
    }

  private:
    std::unique_ptr<Expr> target_;
    std::unique_ptr<Expr> value_;
    SourceSpan operator_span_;
};
class ExprStmt final : public Stmt {
  public:
    ExprStmt(SourceSpan, std::uint16_t, std::unique_ptr<Expr> expr);
    ~ExprStmt() noexcept override;
    void accept(AstVisitor &) const override;
    const Expr &expr() const noexcept {
        return *expr_;
    }

  private:
    std::unique_ptr<Expr> expr_;
};
class BlockStmt final : public Stmt {
  public:
    BlockStmt(SourceSpan, std::uint16_t, std::vector<std::unique_ptr<Stmt>> statements);
    ~BlockStmt() noexcept override;
    void accept(AstVisitor &) const override;
    const ReadOnlyNodes<Stmt> &statements() const noexcept {
        return statements_;
    }

  private:
    ReadOnlyNodes<Stmt> statements_;
};
class IfStmt final : public Stmt {
  public:
    IfStmt(SourceSpan, std::uint16_t, std::unique_ptr<Expr> condition,
           std::unique_ptr<BlockStmt> then_body, std::unique_ptr<Stmt> else_body);
    ~IfStmt() noexcept override;
    void accept(AstVisitor &) const override;
    const Expr &condition() const noexcept {
        return *condition_;
    }
    const BlockStmt &then_body() const noexcept {
        return *then_body_;
    }
    const Stmt *else_body() const noexcept {
        return else_body_.get();
    }

  private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<BlockStmt> then_body_;
    std::unique_ptr<Stmt> else_body_;
};
class WhileStmt final : public Stmt {
  public:
    WhileStmt(SourceSpan, std::uint16_t, std::unique_ptr<Expr> condition,
              std::unique_ptr<BlockStmt> body);
    ~WhileStmt() noexcept override;
    void accept(AstVisitor &) const override;
    const Expr &condition() const noexcept {
        return *condition_;
    }
    const BlockStmt &body() const noexcept {
        return *body_;
    }

  private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<BlockStmt> body_;
};
class ForStmt final : public Stmt {
  public:
    ForStmt(SourceSpan, std::uint16_t, std::string name, SourceSpan name_span,
            std::unique_ptr<Expr> source, std::unique_ptr<BlockStmt> body);
    ~ForStmt() noexcept override;
    void accept(AstVisitor &) const override;
    std::string_view name() const noexcept {
        return name_;
    }
    SourceSpan name_span() const noexcept {
        return name_span_;
    }
    const Expr &source() const noexcept {
        return *source_;
    }
    const BlockStmt &body() const noexcept {
        return *body_;
    }

  private:
    std::string name_;
    SourceSpan name_span_;
    std::unique_ptr<Expr> source_;
    std::unique_ptr<BlockStmt> body_;
};
class ReturnStmt final : public Stmt {
  public:
    ReturnStmt(SourceSpan, std::uint16_t, std::unique_ptr<Expr> value);
    ~ReturnStmt() noexcept override;
    void accept(AstVisitor &) const override;
    const Expr *value() const noexcept {
        return value_.get();
    }

  private:
    std::unique_ptr<Expr> value_;
};
class BreakStmt final : public Stmt {
  public:
    BreakStmt(SourceSpan, std::uint16_t);
    ~BreakStmt() noexcept override;
    void accept(AstVisitor &) const override;

  private:
};
class ContinueStmt final : public Stmt {
  public:
    ContinueStmt(SourceSpan, std::uint16_t);
    ~ContinueStmt() noexcept override;
    void accept(AstVisitor &) const override;

  private:
};
} // namespace myb
