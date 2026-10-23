#pragma once
#include "ast/nodes.hpp"
#include <string>
#include <unordered_map>
#include <variant>
namespace myb {
using LiteralValue = std::variant<std::int64_t, double, std::string>;
using LiteralTable = std::unordered_map<const Node *, LiteralValue>;
struct DeclarationIndex {};
struct CheckedAst {
    const AstProgram *program;
    DeclarationIndex declarations;
    LiteralTable literals;
};
} // namespace myb
