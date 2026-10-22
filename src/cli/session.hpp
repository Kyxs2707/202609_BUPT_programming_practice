#pragma once
#include "builtins/registry.hpp"
#include "bytecode/program.hpp"
#include "cli/options.hpp"
#include "source/source.hpp"
namespace myb {
// Also used by tools embedding the frontend: the returned program lives with this session.
class Session final {
  public:
    Session(EmergencyFailure &, HostIO &, const BuiltinRegistry &);
    ~Session() noexcept;
    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;
    const CompiledProgram &compile_entry(std::string_view);
    int run(const Options &) noexcept;

  private:
    void report(const FailureHeader &, std::string_view) noexcept;
    EmergencyFailure &first_;
    HostIO &io_;
    const BuiltinRegistry &builtins_;
    RootRegistry roots_;
    SourceManager sources_;
    CompiledProgram program_;
    Heap heap_;
};
} // namespace myb
