#pragma once
#include "bytecode/program.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace myb {
struct TestRun {
    int exit_code{};
    std::string stdout_bytes, stderr_bytes;
    std::optional<FailureHeader> failure;
    std::vector<std::string> call_names;
    std::size_t stdin_reads{}, native_calls{};
};
class VmFixture {
  public:
    VmFixture();
    ~VmFixture();
    const CompiledProgram &compile(std::string_view);
    TestRun run(std::string_view);
    TestRun check(std::string_view);

  private:
    struct State;
    TestRun execute(std::string_view, bool);
    std::unique_ptr<State> state_;
};
} // namespace myb
