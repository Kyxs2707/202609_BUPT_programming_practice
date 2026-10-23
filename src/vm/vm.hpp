#pragma once
#include "builtins/registry.hpp"
#include "bytecode/program.hpp"
#include "vm/frame.hpp"
namespace myb {
/**
 * @brief 执行已核验的 M1 顶层字节码，并为活动帧的有效值登记 GC 根。
 *
 * VM 拥有值槽、绑定状态和帧存储，借用堆、根登记表、IO、首错记录及内置表。
 * 这些服务须覆盖 VM 使用期，RootRegistry 须覆盖 VM 析构；登记期间 VM 地址稳定。
 */
class VM final {
  public:
    /** @brief 借用会话服务并登记根回调，不创建活动执行帧。 */
    VM(Heap &, RootRegistry &, HostIO &, EmergencyFailure &, const BuiltinRegistry &) noexcept;
    VM(const VM &) = delete;
    VM &operator=(const VM &) = delete;
    /**
     * @brief 预留值区后执行顶层单元，HALT 或指令执行失败时清理活动帧。
     * @param[in] program 执行期间借用的编译产物及其常量根。
     * @pre VM 无活动帧；program 的顶层单元已核验且执行期间保持不变。
     * @pre program 的常量根和所有受管值均属于本 VM 使用的堆及根登记表。
     * @throws Failure 名称、数量、运算、宿主 IO、资源或 VM 状态检查失败。
     *
     * 内置回调可经 Heap 分配和回收；调用期间参数保留在有效操作数区。
     * 当前执行仅支持顶层表达式及 print/write 等注册回调，不进入用户函数。
     * 执行前的容量准备失败时尚无活动帧，已分配存储由 VM 继续持有。
     */
    void execute(const CompiledProgram &);
    /**
     * @brief 无分配地将所有值置 null、状态置 Dormant，并清空活动帧。
     *
     * 保留已分配容量和根回调登记；帧清空后扫描结果为空，不再由 VM 保活旧值。
     */
    void clear() noexcept;
    /** @brief 清理有效值和帧，随后由成员析构撤销根登记。 */
    ~VM() noexcept {
        clear();
    }

  private:
    /** @brief 标记活动帧的 Initialized 局部槽及 sp 内操作数，不扫描预留空位。 */
    static void trace_values(void *, Marker &);
    /**
     * @brief 将值写入已预留操作数槽，再递增 sp 使其成为根扫描范围的一部分。
     * @pre 存在活动帧；受管输入值在槽位发布前已有根保护。
     *
     * 成功路径不分配或触发 GC；预留槽不足时报告内部状态错误。
     */
    void push(Value);
    /**
     * @brief 检查调用点索引并短期借用该单元的调用点记录。
     * @return 在 unit.calls 存储保持不变期间有效的引用。
     */
    const CallSite &call_site(const CodeUnit &, std::uint32_t);
    /** @brief 以内部状态错误终止执行，由活动指令异常边界补充源码位置。 */
    [[noreturn]] void invalid();
    Heap &heap_;
    RootRegistry &roots_;
    HostIO &io_;
    EmergencyFailure &first_;
    const BuiltinRegistry &builtins_;
    std::vector<Value> values_; ///< 含局部和操作数的预留存储，存储存在不等于槽位有效。
    std::vector<BindingState> states_; ///< 与值区并列的局部绑定状态。
    std::vector<Frame> frames_; ///< 定义根扫描范围及诊断位置的活动帧。
    NativeRootSet root_set_; ///< 借用 VM 稳定地址的根集合登记。
};
} // namespace myb
