#pragma once
namespace myb {
enum class UnaryOp { Plus, Minus, Not };
enum class BinaryOp { Add, Subtract, Multiply, Divide, FloorDivide, Modulo };
enum class CompareOp { Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual };
enum class LogicalOp { And, Or };
} // namespace myb
