#pragma once
#include "bytecode/instruction.hpp"
#include "runtime/heap.hpp"
#include "source/span.hpp"
#include <string>
#include <unordered_map>
#include <vector>
namespace myb {
using NameId = std::uint32_t;
struct CallSite {
    NameId target;
    std::uint32_t arity;
};
struct CodeUnit {
    std::vector<Instruction> instructions;
    std::vector<Value> constants;
    std::vector<SourceSpan> locations;
    std::vector<CallSite> calls;
    std::uint32_t local_count{}, max_operand{};
};
class CompiledProgram final {
  public:
    explicit CompiledProgram(RootRegistry &roots) noexcept : roots_(roots, this, trace_constants) {}
    CompiledProgram(const CompiledProgram &) = delete;
    CompiledProgram &operator=(const CompiledProgram &) = delete;
    void detach_roots() noexcept {
        roots_.detach();
    }
    NameId intern_name(std::string_view, EmergencyFailure &);
    std::vector<std::string> names;
    std::unordered_map<std::string, NameId> name_ids;
    std::vector<CodeUnit> units;
    std::uint32_t top_unit{};

  private:
    static void trace_constants(void *, Marker &);
    NativeRootSet roots_;
};
} // namespace myb
