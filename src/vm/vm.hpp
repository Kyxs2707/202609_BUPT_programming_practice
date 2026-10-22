#pragma once
#include "builtins/registry.hpp"
#include "bytecode/program.hpp"
#include "vm/frame.hpp"
namespace myb {
class VM final {
  public:
    VM(Heap &, RootRegistry &, HostIO &, EmergencyFailure &, const BuiltinRegistry &) noexcept;
    VM(const VM &) = delete;
    VM &operator=(const VM &) = delete;
    void execute(const CompiledProgram &);
    void clear() noexcept;
    ~VM() noexcept {
        clear();
    }

  private:
    static void trace_values(void *, Marker &);
    void push(Value);
    const CallSite &call_site(const CodeUnit &, std::uint32_t);
    [[noreturn]] void invalid();
    Heap &heap_;
    RootRegistry &roots_;
    HostIO &io_;
    EmergencyFailure &first_;
    const BuiltinRegistry &builtins_;
    std::vector<Value> values_;
    std::vector<BindingState> states_;
    std::vector<Frame> frames_;
    NativeRootSet root_set_;
};
} // namespace myb
