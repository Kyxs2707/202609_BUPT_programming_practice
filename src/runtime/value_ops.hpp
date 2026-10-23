#pragma once
#include "diagnostic/failure.hpp"
#include "runtime/operators.hpp"
#include "runtime/value.hpp"
namespace myb {
/**
 * @brief 集中实现当前数值算术与严格 Bool 取反，不分配受管对象或触发 GC。
 * Bool 不参与数值运算；Int64 溢出、除零和非有限浮点结果均报告算术故障。
 */
class ValueOps {
  public:
    /**
     * @brief 对合法数值应用正负号，或对 Bool 应用逻辑取反。
     * @return 运算后的标量值；正号保留原数值表示。
     * @throws MybRuntimeFailure 操作数类型错误，或对 Int64 最小值取负。
     * @throws HostFailure 运算符编号无效。
     */
    static Value unary(UnaryOp, Value, ErrorSite);
    /**
     * @brief 对已求值的左右操作数执行算术，并在结果返回前完成类型和范围检查。
     * 整数加减乘保留 Int64；除法及混合算术提升为 Float64。
     * 整除和余数只接受 Int64，商向下取整，非零余数与除数同号。
     * @return 合法的 Int64 或有限 Float64；允许浮点下溢及带符号零。
     * @throws MybRuntimeFailure 类型错误、除零、整数溢出或浮点结果非有限。
     * @throws HostFailure 运算符编号无效。
     */
    static Value binary(BinaryOp, Value, Value, ErrorSite);
};
} // namespace myb
