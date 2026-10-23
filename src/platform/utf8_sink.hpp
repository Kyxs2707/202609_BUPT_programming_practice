#pragma once
#include <span>
namespace myb {
/** @brief 按显式长度接收 UTF-8 字节的宿主接口，允许保留内嵌 NUL。 */
class Utf8Sink {
  public:
    virtual ~Utf8Sink() noexcept = default;
    /**
     * @brief 在调用期间消费或复制借用的字节范围，失败向调用方传播。
     *
     * 接收方可分配内存；TextBuilder 的增长还可能触发 GC，调用方须事先保活受管值。
     */
    virtual void append(std::span<const char>) = 0;
};
} // namespace myb
