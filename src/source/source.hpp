#pragma once
#include "diagnostic/failure.hpp"
#include "source/span.hpp"
#include <string>
#include <string_view>
#include <vector>
namespace myb {
/** @brief 独占保存源码路径、未经换行转换的原始字节和行索引。 */
struct SourceFile {
    std::string path_utf8, bytes; ///< UTF-8 路径与包含开头 BOM 的源码字节。
    std::vector<std::size_t> line_starts; ///< 各行的起始字节偏移，第一项为 0。
    std::size_t bom_size{}; ///< 开头 UTF-8 BOM 的字节数，取 0 或 3。
};
/**
 * @brief 管理会话源码并将原始字节范围转换为诊断行列。
 *
 * failure_ 由调用者持有，须覆盖管理器寿命。源码记录供解析和诊断借用；
 * add 可能使已有记录引用及其字符串视图失效，调用者应保存 SourceId 后重新获取。
 */
class SourceManager {
  public:
    /** @brief 借用会话首错记录；构造时不读取源码。 */
    explicit SourceManager(EmergencyFailure &first) : failure_(first) {}
    /**
     * @brief 接管路径与源码字节，严格验证 UTF-8 并建立 LF、CRLF、CR 行索引。
     *
     * 编码失败时保留已登记记录及合法前缀的行索引，供首错定位使用。
     * @return 新记录的 SourceId。
     * @throws FrontendFailure 源码含非法 UTF-8，位置指向非法序列的起始字节。
     * @throws HostFailure 源码记录或行索引达到容量上限。
     */
    SourceId add(std::string path_utf8, std::string bytes);
    /**
     * @brief 按编号借用只读源码记录。
     * @return 有效期至下一次 add 或管理器销毁的记录引用。
     * @throws HostFailure 编号不存在。
     */
    const SourceFile &get(SourceId) const;
    /**
     * @brief 将字节偏移映射为行列，开头 BOM 不占列，CRLF 算一次换行。
     *
     * EOF 偏移合法；CRLF 两字节之间的位置归前一行行尾。
     * @throws HostFailure 编号不存在或偏移超过源码字节数。
     */
    SourcePosition locate(SourceId, std::uint64_t) const;
    /**
     * @brief 借用范围内的原始字节，视图有效期至下一次 add 或管理器销毁。
     * @throws HostFailure 编号不存在、范围倒置或越界。
     */
    std::string_view slice(SourceSpan) const;
    /** @brief 借用构造时传入的会话首错记录。 */
    EmergencyFailure &failure() const noexcept {
        return failure_;
    }
    /**
     * @brief 校验两个同源范围，返回从第一个 begin 到第二个 end 的范围。
     * @throws HostFailure 任一范围无效、来源不同或合并后范围倒置。
     */
    SourceSpan merge(SourceSpan, SourceSpan) const;

  private:
    EmergencyFailure &failure_;
    std::vector<SourceFile> files_;
};
} // namespace myb
