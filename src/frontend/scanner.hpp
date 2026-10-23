#pragma once
#include "frontend/driver.hpp"
#include "frontend/flex_allocations.hpp"
#include "parser.hpp"
#include <vector>
typedef void *yyscan_t;
struct yy_buffer_state;
namespace myb {
/**
 * @brief 保存单个可重入扫描器的字节位置、字符串解码状态和分配登记。
 *
 * 通过 Flex 的 yyextra 借用；须覆盖扫描句柄的寿命。
 * consumed 只按规则实际匹配的字节前进，预读不会改变源码位置。
 */
struct ScannerContext {
    SourceId source;
    std::uint64_t consumed{};
    SourceSpan matched{}; ///< 最近一次规则匹配的半开字节范围。
    std::uint64_t string_begin{}; ///< 当前字符串开引号的字节偏移。
    std::string decoded_string; ///< 拥有尚未交付给字符串词法单元的解码结果。
    EmergencyFailure &failure;
    FlexAllocationLedger allocations;
    ScannerContext(SourceId s, EmergencyFailure &f) : source(s), failure(f), allocations(f) {}
    /**
     * @brief 记录本次规则范围并推进消费位置。
     * @param[in] n 本次规则的 yyleng 字节数。
     * @pre 匹配字节属于剩余输入；每次规则动作只调用一次。
     */
    void match(std::size_t n) {
        matched = {source, consumed, consumed + n};
        consumed += n;
    }
    /** @brief 返回消费位置处的空范围，供正常 EOF 词法单元使用。 */
    SourceSpan eof() const noexcept {
        return {source, consumed, consumed};
    }
    /**
     * @brief 以当前 matched 范围报告词法错误并终止扫描。
     * @throws FrontendFailure 故障类别为 Frontend/Syntax。
     */
    [[noreturn]] void lexical_error(std::string_view message) {
        fail({failure, matched}, FailureDomain::Frontend, ReasonCode::Syntax, message);
    }
    /**
     * @brief 将一个已匹配的转义解码并追加至 decoded_string。
     * @pre 输入满足词法规则中的普通转义或 1～6 位 Unicode 转义形状。
     * @throws FrontendFailure Unicode 转义超出标量范围或处于代理区。
     * @throws std::bad_alloc 解码结果扩容失败。
     */
    void escape(std::string_view);
};
/**
 * @brief 独占 Flex 句柄、输入副本及其缓冲状态。
 *
 * 输入副本末尾附加两个 NUL 哨兵，缓冲状态存活时副本地址保持稳定；
 * 源码内部的 NUL 仍作为普通输入处理。析构先销毁扫描状态，
 * 再释放登记中剩余分配，最后由成员析构释放输入副本。
 * Scanner 不可复制或移动，借用的 EmergencyFailure 须覆盖其寿命。
 */
class Scanner {
  public:
    /**
     * @brief 为已登记并验证编码的源码建立扫描状态。
     * @throws HostFailure 输入超过 INT_MAX 或容器容量，或扫描后端初始化失败。
     * @throws std::bad_alloc 输入副本分配失败。
     * 初始化异常会销毁已建立的句柄并清扫剩余分配登记。
     */
    Scanner(SourceManager &, SourceId);
    ~Scanner() noexcept;
    Scanner(const Scanner &) = delete;
    Scanner &operator=(const Scanner &) = delete;
    /**
     * @brief 返回下一个带源码范围的词法单元，正常结束时返回 YYEOF。
     * @return 拥有名称、数字词素或已解码字符串的完整 Bison 符号。
     * @throws FrontendFailure 当前输入含非法词法单元或未闭合字符串。
     * @throws HostFailure 扫描后端发生资源或内部故障。
     * @throws std::bad_alloc 词法单元载荷分配失败。
     */
    Parser::symbol_type next();

  private:
    ScannerContext context_;
    yyscan_t handle_{};
    yy_buffer_state *buffer_{};
    std::vector<char> input_;
};
/**
 * @brief 将 Flex 致命错误转为宿主故障，通过异常返回清理路径。
 * @pre 扫描句柄的 yyextra 指向有效的 ScannerContext。
 * @throws HostFailure 分配或容量错误归 Resource，其余后端错误归 Internal。
 */
[[noreturn]] void flex_fatal(const char *, yyscan_t);
} // namespace myb
/**
 * @brief Flex 生成的可重入词法入口，返回完整 Bison 符号。
 * @pre 句柄及其输入缓冲由仍存活的 Scanner 持有。
 * @return 下一个词法单元或带真实源码末尾范围的 YYEOF。
 */
myb::Parser::symbol_type raw_lex(yyscan_t);
