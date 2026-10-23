#pragma once
#include "bytecode/program.hpp"
namespace myb {
/**
 * @brief 核验 M1 线性指令及其操作数栈效果，成功后写入最大操作数深度。
 * @param[in,out] unit 待核验单元，成功时更新 max_operand。
 * @param[in,out] first 失败时记录首个错误。
 * @throws Failure 指令、相关索引、栈深或终止结构无效，或最大栈深超限。
 *
 * 核验包含逐指令位置表长度、b 为零、支持的操作码及 HALT 后无指令。
 * PRECALL 和 CALL 各自核验调用点索引，不检查二者配对或先后关系；
 * 预检先于实参的顺序由 BytecodeCompiler 生成过程保证。
 * 程序名称编号和运行值检查留给 VM，不从这里提前报告语言运行错误。
 */
void verify(CodeUnit &, EmergencyFailure &);
}
