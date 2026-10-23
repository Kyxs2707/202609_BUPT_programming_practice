#pragma once
#include "ast/nodes.hpp"
#include <string>
#include <unordered_map>
#include <variant>
namespace myb {
/** @brief 已规范化的宿主字面量数据，字符串拥有存储，均不含受管堆引用。 */
using LiteralValue = std::variant<std::int64_t, double, std::string>;
/**
 * @brief 按借用节点地址索引的字面量侧表。
 * 常规字面量以自身为键；Int64 最小值的直接负号例外以 UnaryExpr 为键。
 * 键所属的 AST 须在读取侧表期间保持存活。
 */
using LiteralTable = std::unordered_map<const Node *, LiteralValue>;
/** @brief 为后续声明登记保留的位置；当前 M1 不支持声明，此记录为空。 */
struct DeclarationIndex {};
/**
 * @brief 借用已检查的 AST，并拥有本次检查生成的侧表。
 *
 * 当前表示通过 M1 检查的结果。program 及 literals 的节点键借用同一棵树，
 * 调用方须使树存活至结果最后一次使用；销毁本结果不会销毁树。
 * 字节码生成完成后可释放本结果和 AST，生成产物须拥有独立的数据。
 */
struct CheckedAst {
    const AstProgram *program; ///< 非空程序借用，由 Checker::check 填入。
    DeclarationIndex declarations; ///< 本结果拥有的声明记录。
    LiteralTable literals; ///< 本结果拥有的规范化字面量。
};
} // namespace myb
