%option reentrant noyywrap noinput nounput nodefault noyyalloc noyyrealloc noyyfree never-interactive
%option extra-type="myb::ScannerContext*"
%x DQUOTE SQUOTE
%{
#include "frontend/scanner.hpp"
#include <string_view>
// M1 只扫描已实现的表达式子集；其余语言关键字在首错处终止。
// 名称和数字词素复制后交付；字符串状态交付完整范围和已解码文本。
#define YY_DECL myb::Parser::symbol_type raw_lex(yyscan_t yyscanner)
// 空白、注释和 BOM 也按实际匹配字节推进，避免预读影响源码位置。
#define YY_USER_ACTION yyextra->match(static_cast<std::size_t>(yyleng));
// 后端错误通过异常展开，交由 Scanner 清理句柄和分配登记。
#define YY_FATAL_ERROR(message) myb::flex_fatal(message,yyscanner)
// 完整 symbol 接口的默认终止路径也须返回带范围的 EOF。
#define yyterminate() return myb::Parser::make_YYEOF(yyextra->eof())
using myb::Parser;
%}
DIGIT [0-9]
EXP [eE][+-]?{DIGIT}+
FLOATING {DIGIT}+\.{DIGIT}+({EXP})?|{DIGIT}+{EXP}
IDENT [A-Za-z_][A-Za-z_0-9]*
UTF8 [\xC2-\xDF][\x80-\xBF]|[\xE0-\xEF][\x80-\xBF]{2}|[\xF0-\xF4][\x80-\xBF]{3}
 /* UTF-8 已由 SourceManager 验证；非法源码字符按完整码点匹配。 */
%%
\xEF\xBB\xBF {if(yyextra->matched.begin!=0)yyextra->lexical_error("unexpected BOM");}
[ \t\r\n]+ {}
#[^\r\n]* {}
"true" {return Parser::make_TRUE(yyextra->matched);}
"false" {return Parser::make_FALSE(yyextra->matched);}
"null" {return Parser::make_NULL_VALUE(yyextra->matched);}
"let"|"if"|"else"|"while"|"for"|"in"|"return"|"break"|"continue"|"fn"|"struct" {yyextra->lexical_error("syntax is unavailable in the M1 subset");}
{IDENT} {return Parser::make_IDENTIFIER(std::string(yytext,yyleng),yyextra->matched);}
{FLOATING} {return Parser::make_FLOAT(myb::FloatLexeme{std::string(yytext,yyleng)},yyextra->matched);}
{DIGIT}+ {return Parser::make_INT(myb::IntLexeme{std::string(yytext,yyleng)},yyextra->matched);}
{DIGIT}+[A-Za-z_0-9.]*|{DIGIT}+(\.{DIGIT}*)?[eE][+-]?[A-Za-z_0-9.]* {yyextra->lexical_error("invalid number token");}
"++"|"--"|"+="|"-="|"*="|"/="|"%="|"//=" {yyextra->lexical_error("unsupported operator");}
"+" {return Parser::make_PLUS(yyextra->matched);}
"-" {return Parser::make_MINUS(yyextra->matched);}
"*" {return Parser::make_STAR(yyextra->matched);}
"/" {return Parser::make_SLASH(yyextra->matched);}
"//" {return Parser::make_FLOOR(yyextra->matched);}
"%" {return Parser::make_PERCENT(yyextra->matched);}
"!" {return Parser::make_NOT(yyextra->matched);}
"(" {return Parser::make_LPAREN(yyextra->matched);}
")" {return Parser::make_RPAREN(yyextra->matched);}
"," {return Parser::make_COMMA(yyextra->matched);}
";" {return Parser::make_SEMICOLON(yyextra->matched);}
\" {yyextra->string_begin=yyextra->matched.begin;yyextra->decoded_string.clear();BEGIN(DQUOTE);}
\' {yyextra->string_begin=yyextra->matched.begin;yyextra->decoded_string.clear();BEGIN(SQUOTE);}
<DQUOTE>\" {BEGIN(INITIAL);return Parser::make_STRING(std::move(yyextra->decoded_string),{yyextra->source,yyextra->string_begin,yyextra->consumed});}
<SQUOTE>\' {BEGIN(INITIAL);return Parser::make_STRING(std::move(yyextra->decoded_string),{yyextra->source,yyextra->string_begin,yyextra->consumed});}
<DQUOTE>[^\\\"\r\n]+ {yyextra->decoded_string.append(yytext,yyleng);}
<SQUOTE>[^\\\'\r\n]+ {yyextra->decoded_string.append(yytext,yyleng);}
<DQUOTE,SQUOTE>\\u\{[0-9A-Fa-f]{1,6}\}|\\[\\\"\'nrt] {yyextra->escape(std::string_view(yytext,yyleng));}
<DQUOTE,SQUOTE>\\(.|\r|\n) {yyextra->lexical_error("invalid string escape");}
<DQUOTE,SQUOTE>\r\n|\r|\n {yyextra->lexical_error("string cannot cross a source newline");}
<DQUOTE,SQUOTE>\\ {yyextra->lexical_error("unfinished string escape");}
<DQUOTE,SQUOTE><<EOF>> {yyextra->matched={yyextra->source,yyextra->string_begin,yyextra->consumed};yyextra->lexical_error("unterminated string");}
{UTF8} {yyextra->lexical_error("invalid source character");}
. {yyextra->lexical_error("invalid source character");}
<<EOF>> {return Parser::make_YYEOF(yyextra->eof());}
%%
