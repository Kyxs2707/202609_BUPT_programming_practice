#pragma once
#include "diagnostic/failure.hpp"
#include "runtime/value.hpp"
namespace myb {
class TextBuilder;
/**
 * @brief 把 null、Bool、Int64、Float64 或 String 的 MYB 显示文本追加到 Builder。
 * 字符串按原字节输出；浮点使用 DecimalCodec，当前容器值报告内部故障。
 * @pre 输入对象属于输出 Builder 的会话，并在整个调用期间已有根保护。
 * Builder 扩容可触发 GC，传入的 Value 副本本身不构成根。
 * @throws HostFailure 容器值、内部格式化故障、容量或宿主分配失败。
 * @throws std::bad_alloc 浮点格式化使用的宿主字符串分配失败。
 */
void display(Value, TextBuilder &, ErrorSite);
} // namespace myb
