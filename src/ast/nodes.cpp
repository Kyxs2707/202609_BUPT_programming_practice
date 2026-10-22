#include "ast/nodes.hpp"
namespace myb {
AstProgram::AstProgram(SourceSpan s, std::uint16_t h, std::vector<std::unique_ptr<TopItem>> items)
    : Node(s, h), items_(std::move(items)) {}
AstProgram::~AstProgram() noexcept = default;
void AstProgram::accept(AstVisitor &v) const {
    v.visit(*this);
}
FunctionDecl::FunctionDecl(SourceSpan s, std::uint16_t h, std::string name, SourceSpan name_span,
                           std::vector<Parameter> parameters, std::unique_ptr<BlockStmt> body)
    : TopItem(s, h), name_(std::move(name)), name_span_(std::move(name_span)),
      parameters_(std::move(parameters)), body_(std::move(body)) {}
FunctionDecl::~FunctionDecl() noexcept = default;
void FunctionDecl::accept(AstVisitor &v) const {
    v.visit(*this);
}
StructDecl::StructDecl(SourceSpan s, std::uint16_t h, std::string name, SourceSpan name_span,
                       std::vector<FieldDecl> fields)
    : TopItem(s, h), name_(std::move(name)), name_span_(std::move(name_span)),
      fields_(std::move(fields)) {}
StructDecl::~StructDecl() noexcept = default;
void StructDecl::accept(AstVisitor &v) const {
    v.visit(*this);
}
IntExpr::IntExpr(SourceSpan s, std::uint16_t h, std::string digits)
    : Expr(s, h), digits_(std::move(digits)) {}
IntExpr::~IntExpr() noexcept = default;
void IntExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
FloatExpr::FloatExpr(SourceSpan s, std::uint16_t h, std::string text)
    : Expr(s, h), text_(std::move(text)) {}
FloatExpr::~FloatExpr() noexcept = default;
void FloatExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
StringExpr::StringExpr(SourceSpan s, std::uint16_t h, std::string value)
    : Expr(s, h), value_(std::move(value)) {}
StringExpr::~StringExpr() noexcept = default;
void StringExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
BoolExpr::BoolExpr(SourceSpan s, std::uint16_t h, bool value)
    : Expr(s, h), value_(std::move(value)) {}
BoolExpr::~BoolExpr() noexcept = default;
void BoolExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
NullExpr::NullExpr(SourceSpan s, std::uint16_t h) : Expr(s, h) {}
NullExpr::~NullExpr() noexcept = default;
void NullExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
NameExpr::NameExpr(SourceSpan s, std::uint16_t h, std::string name, SourceSpan name_span)
    : Expr(s, h), name_(std::move(name)), name_span_(std::move(name_span)) {}
NameExpr::~NameExpr() noexcept = default;
void NameExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
GroupExpr::GroupExpr(SourceSpan s, std::uint16_t h, std::unique_ptr<Expr> expr)
    : Expr(s, h), expr_(std::move(expr)) {}
GroupExpr::~GroupExpr() noexcept = default;
void GroupExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
UnaryExpr::UnaryExpr(SourceSpan s, std::uint16_t h, UnaryOp op, SourceSpan operator_span,
                     std::unique_ptr<Expr> operand)
    : Expr(s, h), op_(std::move(op)), operator_span_(std::move(operator_span)),
      operand_(std::move(operand)) {}
UnaryExpr::~UnaryExpr() noexcept = default;
void UnaryExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
BinaryExpr::BinaryExpr(SourceSpan s, std::uint16_t h, ArithmeticOrCompare op,
                       SourceSpan operator_span, std::unique_ptr<Expr> left,
                       std::unique_ptr<Expr> right)
    : Expr(s, h), op_(std::move(op)), operator_span_(std::move(operator_span)),
      left_(std::move(left)), right_(std::move(right)) {}
BinaryExpr::~BinaryExpr() noexcept = default;
void BinaryExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
LogicalExpr::LogicalExpr(SourceSpan s, std::uint16_t h, LogicalOp op, SourceSpan operator_span,
                         std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
    : Expr(s, h), op_(std::move(op)), operator_span_(std::move(operator_span)),
      left_(std::move(left)), right_(std::move(right)) {}
LogicalExpr::~LogicalExpr() noexcept = default;
void LogicalExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
CallExpr::CallExpr(SourceSpan s, std::uint16_t h, std::string name, SourceSpan name_span,
                   std::vector<std::unique_ptr<Expr>> args)
    : Expr(s, h), name_(std::move(name)), name_span_(std::move(name_span)), args_(std::move(args)) {
}
CallExpr::~CallExpr() noexcept = default;
void CallExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
ArrayExpr::ArrayExpr(SourceSpan s, std::uint16_t h, std::vector<std::unique_ptr<Expr>> elements)
    : Expr(s, h), elements_(std::move(elements)) {}
ArrayExpr::~ArrayExpr() noexcept = default;
void ArrayExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
ConstructExpr::ConstructExpr(SourceSpan s, std::uint16_t h, std::string name, SourceSpan name_span,
                             std::vector<FieldInit> fields)
    : Expr(s, h), name_(std::move(name)), name_span_(std::move(name_span)),
      fields_(std::move(fields)) {}
ConstructExpr::~ConstructExpr() noexcept = default;
void ConstructExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
FieldExpr::FieldExpr(SourceSpan s, std::uint16_t h, std::unique_ptr<Expr> receiver,
                     std::string name, SourceSpan name_span, SourceSpan access_span)
    : Expr(s, h), receiver_(std::move(receiver)), name_(std::move(name)),
      name_span_(std::move(name_span)), access_span_(std::move(access_span)) {}
FieldExpr::~FieldExpr() noexcept = default;
void FieldExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
IndexExpr::IndexExpr(SourceSpan s, std::uint16_t h, std::unique_ptr<Expr> receiver,
                     std::unique_ptr<Expr> index, SourceSpan access_span)
    : Expr(s, h), receiver_(std::move(receiver)), index_(std::move(index)),
      access_span_(std::move(access_span)) {}
IndexExpr::~IndexExpr() noexcept = default;
void IndexExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
SliceExpr::SliceExpr(SourceSpan s, std::uint16_t h, std::unique_ptr<Expr> receiver,
                     std::unique_ptr<Expr> start, std::unique_ptr<Expr> end, SourceSpan access_span)
    : Expr(s, h), receiver_(std::move(receiver)), start_(std::move(start)), end_(std::move(end)),
      access_span_(std::move(access_span)) {}
SliceExpr::~SliceExpr() noexcept = default;
void SliceExpr::accept(AstVisitor &v) const {
    v.visit(*this);
}
LetStmt::LetStmt(SourceSpan s, std::uint16_t h, std::string name, SourceSpan name_span,
                 std::unique_ptr<Expr> initializer)
    : Stmt(s, h), name_(std::move(name)), name_span_(std::move(name_span)),
      initializer_(std::move(initializer)) {}
LetStmt::~LetStmt() noexcept = default;
void LetStmt::accept(AstVisitor &v) const {
    v.visit(*this);
}
AssignStmt::AssignStmt(SourceSpan s, std::uint16_t h, std::unique_ptr<Expr> target,
                       std::unique_ptr<Expr> value, SourceSpan operator_span)
    : Stmt(s, h), target_(std::move(target)), value_(std::move(value)),
      operator_span_(std::move(operator_span)) {}
AssignStmt::~AssignStmt() noexcept = default;
void AssignStmt::accept(AstVisitor &v) const {
    v.visit(*this);
}
ExprStmt::ExprStmt(SourceSpan s, std::uint16_t h, std::unique_ptr<Expr> expr)
    : Stmt(s, h), expr_(std::move(expr)) {}
ExprStmt::~ExprStmt() noexcept = default;
void ExprStmt::accept(AstVisitor &v) const {
    v.visit(*this);
}
BlockStmt::BlockStmt(SourceSpan s, std::uint16_t h, std::vector<std::unique_ptr<Stmt>> statements)
    : Stmt(s, h), statements_(std::move(statements)) {}
BlockStmt::~BlockStmt() noexcept = default;
void BlockStmt::accept(AstVisitor &v) const {
    v.visit(*this);
}
IfStmt::IfStmt(SourceSpan s, std::uint16_t h, std::unique_ptr<Expr> condition,
               std::unique_ptr<BlockStmt> then_body, std::unique_ptr<Stmt> else_body)
    : Stmt(s, h), condition_(std::move(condition)), then_body_(std::move(then_body)),
      else_body_(std::move(else_body)) {}
IfStmt::~IfStmt() noexcept = default;
void IfStmt::accept(AstVisitor &v) const {
    v.visit(*this);
}
WhileStmt::WhileStmt(SourceSpan s, std::uint16_t h, std::unique_ptr<Expr> condition,
                     std::unique_ptr<BlockStmt> body)
    : Stmt(s, h), condition_(std::move(condition)), body_(std::move(body)) {}
WhileStmt::~WhileStmt() noexcept = default;
void WhileStmt::accept(AstVisitor &v) const {
    v.visit(*this);
}
ForStmt::ForStmt(SourceSpan s, std::uint16_t h, std::string name, SourceSpan name_span,
                 std::unique_ptr<Expr> source, std::unique_ptr<BlockStmt> body)
    : Stmt(s, h), name_(std::move(name)), name_span_(std::move(name_span)),
      source_(std::move(source)), body_(std::move(body)) {}
ForStmt::~ForStmt() noexcept = default;
void ForStmt::accept(AstVisitor &v) const {
    v.visit(*this);
}
ReturnStmt::ReturnStmt(SourceSpan s, std::uint16_t h, std::unique_ptr<Expr> value)
    : Stmt(s, h), value_(std::move(value)) {}
ReturnStmt::~ReturnStmt() noexcept = default;
void ReturnStmt::accept(AstVisitor &v) const {
    v.visit(*this);
}
BreakStmt::BreakStmt(SourceSpan s, std::uint16_t h) : Stmt(s, h) {}
BreakStmt::~BreakStmt() noexcept = default;
void BreakStmt::accept(AstVisitor &v) const {
    v.visit(*this);
}
ContinueStmt::ContinueStmt(SourceSpan s, std::uint16_t h) : Stmt(s, h) {}
ContinueStmt::~ContinueStmt() noexcept = default;
void ContinueStmt::accept(AstVisitor &v) const {
    v.visit(*this);
}
} // namespace myb
