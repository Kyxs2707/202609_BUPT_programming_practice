#include "bytecode/program.hpp"
#include <limits>
namespace myb {
void CompiledProgram::trace_constants(void *self, Marker &marker) {
    for (const auto &unit : static_cast<CompiledProgram *>(self)->units)
        for (Value value : unit.constants)
            marker.mark(value);
}
NameId CompiledProgram::intern_name(std::string_view name, EmergencyFailure &first) {
    auto found = name_ids.find(std::string(name));
    if (found != name_ids.end())
        return found->second;
    if (names.size() >= std::numeric_limits<NameId>::max())
        fail({first, {}}, FailureDomain::Resource, ReasonCode::Capacity, "too many program names");
    const auto id = static_cast<NameId>(names.size());
    // 编号引用拥有内容的名称池，编译结束后不依赖 AST 名称的寿命。
    names.emplace_back(name);
    name_ids.emplace(names.back(), id);
    return id;
}
} // namespace myb
