#pragma once
namespace myb {
/** @brief 标识数值正负号及严格 Bool 逻辑取反。 */
enum class UnaryOp { Plus, Minus, Not };
/** @brief 标识二元算术；整除和余数采用向下取整的商。 */
enum class BinaryOp { Add, Subtract, Multiply, Divide, FloorDivide, Modulo };
/** @brief 标识相等判断及有序比较，由各值类型的比较规则解释。 */
enum class CompareOp { Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual };
/** @brief 标识短路逻辑运算，求值顺序由执行方控制。 */
enum class LogicalOp { And, Or };
} // namespace myb
