%skeleton "lalr1.cc"
%require "3.8"
%define api.namespace {myb}
%define api.parser.class {Parser}
%define api.value.type variant
%define api.token.constructor
%define parse.error detailed
%locations
%define api.location.type {myb::SourceSpan}
%parse-param {myb::Driver& driver}
%lex-param {myb::Driver& driver}
%code requires {
#include "frontend/driver.hpp"
// 非空归约覆盖首尾符号；空归约沿用前一符号末尾的空范围。
#define YYLLOC_DEFAULT(Current,Rhs,N) do { if(N){ (Current)={YYRHSLOC(Rhs,1).source,YYRHSLOC(Rhs,1).begin,YYRHSLOC(Rhs,N).end};}else{(Current)={YYRHSLOC(Rhs,0).source,YYRHSLOC(Rhs,0).end,YYRHSLOC(Rhs,0).end};}}while(false)
}
%code provides {namespace myb {
/**
 * @brief 将 Bison 的词法请求交给当前驱动的扫描器。
 * 驱动参数持有本次解析的扫描状态。
 * @return 拥有载荷并携带源码范围的下一个符号。
 */
Parser::symbol_type yylex(Driver&);}}
// 为首个空归约和空文件提供有效的 SourceId 及起点。
%initial-action { @$={driver.source_id(),0,0}; }
%token <std::string> IDENTIFIER STRING
%token <myb::IntLexeme> INT
%token <myb::FloatLexeme> FLOAT
%token TRUE FALSE NULL_VALUE
%token PLUS "+" MINUS "-" STAR "*" SLASH "/" FLOOR "//" PERCENT "%" NOT "!"
%token LPAREN "(" RPAREN ")" COMMA "," SEMICOLON ";"
%left PLUS MINUS
%left STAR SLASH FLOOR PERCENT
%precedence PREFIX
%type <std::vector<std::unique_ptr<myb::TopItem>>> items
%type <std::unique_ptr<myb::Stmt>> statement
%type <std::unique_ptr<myb::Expr>> expression
%type <std::vector<std::unique_ptr<myb::Expr>>> arguments argument_list
%start program
/* M1 文法只归约表达式语句；各动作移动所有权，工厂在连接父节点前检查高度。 */
%%
program: items { driver.accept(driver.factory().make<myb::AstProgram>(@$,std::move($1))); };
items: %empty { $$=std::vector<std::unique_ptr<myb::TopItem>>{}; }
 | items statement {$$=std::move($1);$$.push_back(std::move($2));};
statement: expression SEMICOLON {$$=driver.factory().make<myb::ExprStmt>(@$,std::move($1));};
arguments: %empty {$$=std::vector<std::unique_ptr<myb::Expr>>{};} | argument_list {$$=std::move($1);};
argument_list: expression {$$=std::vector<std::unique_ptr<myb::Expr>>{};$$.push_back(std::move($1));}
 | argument_list COMMA expression {$$=std::move($1);$$.push_back(std::move($3));};
expression:
 INT {$$=driver.factory().make<myb::IntExpr>(@$,std::move($1.digits));}
 | FLOAT {$$=driver.factory().make<myb::FloatExpr>(@$,std::move($1.text));}
 | STRING {$$=driver.factory().make<myb::StringExpr>(@$,std::move($1));}
 | TRUE {$$=driver.factory().make<myb::BoolExpr>(@$,true);}
 | FALSE {$$=driver.factory().make<myb::BoolExpr>(@$,false);}
 | NULL_VALUE {$$=driver.factory().make<myb::NullExpr>(@$);}
 | IDENTIFIER {$$=driver.factory().make<myb::NameExpr>(@$,std::move($1),@1);}
 | LPAREN expression RPAREN {$$=driver.factory().make<myb::GroupExpr>(@$,std::move($2));}
 | IDENTIFIER LPAREN arguments RPAREN {$$=driver.factory().make<myb::CallExpr>(@$,std::move($1),@1,std::move($3));}
 | PLUS expression %prec PREFIX {$$=driver.factory().make<myb::UnaryExpr>(@$,myb::UnaryOp::Plus,@1,std::move($2));}
 | MINUS expression %prec PREFIX {$$=driver.factory().make<myb::UnaryExpr>(@$,myb::UnaryOp::Minus,@1,std::move($2));}
 | NOT expression %prec PREFIX {$$=driver.factory().make<myb::UnaryExpr>(@$,myb::UnaryOp::Not,@1,std::move($2));}
 | expression PLUS expression {$$=driver.factory().make<myb::BinaryExpr>(@$,myb::ArithmeticOrCompare{myb::BinaryOp::Add},@2,std::move($1),std::move($3));}
 | expression MINUS expression {$$=driver.factory().make<myb::BinaryExpr>(@$,myb::ArithmeticOrCompare{myb::BinaryOp::Subtract},@2,std::move($1),std::move($3));}
 | expression STAR expression {$$=driver.factory().make<myb::BinaryExpr>(@$,myb::ArithmeticOrCompare{myb::BinaryOp::Multiply},@2,std::move($1),std::move($3));}
 | expression SLASH expression {$$=driver.factory().make<myb::BinaryExpr>(@$,myb::ArithmeticOrCompare{myb::BinaryOp::Divide},@2,std::move($1),std::move($3));}
 | expression FLOOR expression {$$=driver.factory().make<myb::BinaryExpr>(@$,myb::ArithmeticOrCompare{myb::BinaryOp::FloorDivide},@2,std::move($1),std::move($3));}
 | expression PERCENT expression {$$=driver.factory().make<myb::BinaryExpr>(@$,myb::ArithmeticOrCompare{myb::BinaryOp::Modulo},@2,std::move($1),std::move($3));}
 ;
%%
// 首个语法错误直接传播，Bison 栈中的 variant 与 unique_ptr 随展开释放。
void myb::Parser::error(const location_type& where,const std::string& text){myb::fail({driver.failure(),where},myb::FailureDomain::Frontend,myb::ReasonCode::Syntax,text);}
