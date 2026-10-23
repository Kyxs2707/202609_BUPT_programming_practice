#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace myb {
class EmergencyFailure;
/** @brief 区分成功、十进制文本内容不合法及可表示范围超限。 */
enum class NumericParseError { None, Content, Range };
/** @brief 区分严格源码浮点词素与允许 ASCII 首尾空白和符号的文本转换。 */
enum class ParseMode { SourceLiteral, Conversion };
/**
 * @brief 返回解析标量或受控文本错误，由调用方决定语言故障或空结果的处理。
 * @tparam T 解析得到的数值类型。
 * 成功时 value 有值且 error 为 None；失败时 value 为空，不写入会话首错记录。
 */
template <class T> struct NumericParseResult {
    std::optional<T> value;
    NumericParseError error = NumericParseError::None;
};
/**
 * @brief 提供与 locale 无关的十进制解析及 MYB 浮点显示格式。
 * 解析采用 fast_float，最短回读数字由 Ryu 生成，再按 MYB 规则排版。
 * 本模块不构造受管对象、不触发 GC；宿主和内部故障直接传播。
 */
class DecimalCodec {
  public:
    /**
     * @brief 设置并验证最近值、正中取偶的浮点舍入环境。
     * @pre 在会话数值操作前调用；后续代码保持 FE_TONEAREST。
     * @throws HostFailure 无法设置或验证所需舍入模式。
     */
    static void initialize(EmergencyFailure &);
    /**
     * @brief 完整校验十进制文法后解析有限 Float64，保留正负零并接受下溢到零。
     * SourceLiteral 要求小数点或指数且无首尾空白和前导符号；Conversion 裁去
     * ASCII 9 至 13、32 空白，允许一个正负号，也接受十进制整数文本。
     * @return 成功值，或 Content、Range；受控文本错误不抛出语言异常。
     */
    static NumericParseResult<double> parse_float(std::string_view, ParseMode);
    /**
     * @brief 裁去 ASCII 首尾空白后解析可带正负号的十进制 Int64。
     * 按无符号幅值检查范围，可直接表示 Int64 最小值，允许前导零。
     * @return 成功值，或 Content、Range；受控文本错误不记录会话首错。
     */
    static NumericParseResult<std::int64_t> parse_int(std::string_view);
    /**
     * @brief 生成有限 Float64 的最短回读文本，并应用 MYB 的定点和科学记法规则。
     * 十进制科学指数位于 [-4, 16) 时用定点；其他值用小写 e 和显式非负指数符号。
     * 零分别显示为 0.0 或 -0.0，定点整数保留 .0。
     * @return 拥有存储的显示文本。
     * @throws HostFailure 输入非有限，或 Ryu 输出不符合内部约定。
     * @throws std::bad_alloc 宿主字符串分配失败。
     */
    static std::string format_float(double);
};
} // namespace myb
