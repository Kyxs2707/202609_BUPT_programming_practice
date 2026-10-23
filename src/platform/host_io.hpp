#pragma once
#include "diagnostic/failure.hpp"
#include "platform/utf8_sink.hpp"
#include <string>
#include <string_view>
namespace myb {
/** @brief 入口读取结果，独占保存解析后的 UTF-8 路径与未经文本转换的源码字节。 */
struct EntryBytes {
    std::string path_utf8, bytes;
};
/** @brief 文本行读取状态，区分一行内容与输入结束。 */
enum class LineStatus { Line, End };
/**
 * @brief 隔离核心与平台 I/O，只交换宿主文本和字节，不持有受管对象。
 *
 * 传入视图只在调用期间借用；Utf8Sink 的存储增长可触发 GC，受管值由上层保活。
 */
class HostIO {
  public:
    virtual ~HostIO() noexcept = default;
    /** @brief 读取入口的原始字节并返回拥有存储的路径与内容。 */
    virtual EntryBytes read_entry(std::string_view) = 0;
    /** @brief 预留的行读取接口，M1 的 Windows 实现以 Internal/Invariant 拒绝调用。 */
    virtual LineStatus read_line(Utf8Sink &) = 0;
    /** @brief 预留的文本文件读取接口，M1 的 Windows 实现以 Internal/Invariant 拒绝调用。 */
    virtual void read_text(std::string_view, Utf8Sink &) = 0;
    /** @brief 预留的文本文件写入接口，M1 的 Windows 实现以 Internal/Invariant 拒绝调用。 */
    virtual void write_text(std::string_view, std::string_view) = 0;
    /** @brief 写出全部 stdout 字节，并按 append_lf 决定是否额外写一个 LF。 */
    virtual void write_stdout(std::string_view, bool append_lf) = 0;
    /** @brief 写出全部 stderr 字节，失败交由诊断调用方处理。 */
    virtual void write_stderr(std::string_view) = 0;
};
/**
 * @brief 严格将 UTF-8 转为 Windows UTF-16，保留内嵌 NUL，空串返回空串。
 * @throws HostFailure 输入超过 INT_MAX 为 Resource/Capacity，编码或转换失败为 Internal/Encoding。
 */
std::wstring utf8_to_wide(std::string_view, ErrorSite);
/**
 * @brief 严格将 Windows UTF-16 转为 UTF-8，拒绝孤立代理项并保留内嵌 NUL。
 * @throws HostFailure 输入超过 INT_MAX 为 Resource/Capacity，编码或转换失败为 Internal/Encoding。
 */
std::string wide_to_utf8(std::wstring_view, ErrorSite);
/**
 * @brief 使用 Windows 宽路径读取源码并按句柄类型写标准流的 M1 平台实现。
 *
 * 借用的首错记录须覆盖本对象寿命。入口路径拒绝 NUL 并解析为绝对路径；
 * 源码编码由 SourceManager 验证。控制台输出严格转 UTF-16，文件和管道按原字节写尽。
 * 标准流由宿主持有，本对象不关闭；read_line、read_text、write_text 留待后续阶段实现。
 */
class WindowsHostIO final : public HostIO {
  public:
    /** @brief 绑定会话首错记录，不预读标准流。 */
    explicit WindowsHostIO(EmergencyFailure &failure) : failure_(failure) {}
    EntryBytes read_entry(std::string_view) override;
    LineStatus read_line(Utf8Sink &) override;
    void read_text(std::string_view, Utf8Sink &) override;
    void write_text(std::string_view, std::string_view) override;
    void write_stdout(std::string_view, bool append_lf) override;
    void write_stderr(std::string_view) override;

  private:
    EmergencyFailure &failure_;
};
} // namespace myb
