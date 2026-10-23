#pragma once
#include "platform/utf8_sink.hpp"
#include <string>
#include <string_view>

namespace myb {
class Heap;
/**
 * @brief 分段构造文本，并把实际缓冲容量计入所属 Heap 的增长压力。
 * 本对象借用 Heap，须先于 Heap 销毁；自身不登记源字符串或其他受管值的根。
 * reserve 和 append 的容量增长可触发 GC，调用方须为输入对象保活。
 * 成功交给 Heap::make_string 后缓冲已转移，后续只销毁本对象。
 */
class TextBuilder final : public Utf8Sink {
  public:
    /** @brief 借用 Heap 并登记初始缓冲容量，容量累计溢出时报告资源故障。 */
    explicit TextBuilder(Heap &);
    ~TextBuilder() noexcept override;
    TextBuilder(const TextBuilder &) = delete;
    TextBuilder &operator=(const TextBuilder &) = delete;
    /**
     * @brief 按字节长度追加输入，允许输入含 NUL 或借用当前缓冲自身。
     * 输入来自受管对象时，调用方须让该对象在可能触发的 GC 中保持可达。
     * 本入口不校验 UTF-8；合法性在字符串发布时统一检查。
     * @throws HostFailure 已消费、容量超限或宿主分配失败。
     */
    void append(std::span<const char>) override;
    /**
     * @brief 预留至少指定字节容量，按实际容量修正记账；内容长度保持不变。
     * 容量增长可触发 GC，并使 bytes 返回的旧视图失效。
     * @throws HostFailure 已消费、容量超限或宿主分配失败。
     */
    void reserve(std::size_t);
    /**
     * @brief 借用当前缓冲字节，不转移所有权。
     * @return 有效期至缓冲扩容、缓冲转移或本对象销毁的视图。
     * 调用 append 后须重新取得视图，以取得当前内容和长度。
     */
    std::string_view bytes() const noexcept {
        return buffer_;
    }

  private:
    friend class Heap;
    Heap &heap_;
    std::string buffer_;
    std::size_t accounted_capacity_{}; ///< 当前由本 Builder 承担、析构时需扣除的容量。
    bool consumed_{}; ///< 缓冲是否已成功交接给 StringObject。
};
} // namespace myb
