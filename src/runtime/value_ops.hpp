#pragma once
#include "diagnostic/failure.hpp"
#include "runtime/operators.hpp"
#include "runtime/value.hpp"
namespace myb {
class ValueOps {
  public:
    static Value unary(UnaryOp, Value, ErrorSite);
    static Value binary(BinaryOp, Value, Value, ErrorSite);
};
} // namespace myb
