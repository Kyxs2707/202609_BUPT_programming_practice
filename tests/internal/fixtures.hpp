#pragma once
#include "bytecode/program.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace myb {
/** @brief 独占保存一次会话的退出状态、原始输出字节与调用计数，供内部测试断言。 */
struct TestRun {
    int exit_code{};
    std::string stdout_bytes, stderr_bytes;
    std::optional<FailureHeader> failure;
    std::vector<std::string> call_names;
    std::size_t stdin_reads{}, native_calls{};
};
/**
 * @brief 使用内存 HostIO 运行真实 Session 的内部测试辅助对象。
 *
 * 每次 compile、run、check 都替换内部会话，之前借用的程序及其视图随之失效。
 */
class VmFixture {
  public:
    VmFixture();
    ~VmFixture();
    /** @brief 创建新会话并编译源码，返回借用的已核验程序；失败直接传播异常。 */
    const CompiledProgram &compile(std::string_view);
    /** @brief 创建新会话执行源码，返回独立保存的结果和调用计数。 */
    TestRun run(std::string_view);
    /** @brief 创建新会话检查源码，记录输出与计数以验证检查模式没有执行副作用。 */
    TestRun check(std::string_view);

  private:
    struct State;
    TestRun execute(std::string_view, bool);
    std::unique_ptr<State> state_;
};
} // namespace myb
