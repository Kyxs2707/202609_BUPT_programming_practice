#include "builtins/registry.hpp"
namespace myb {
const BuiltinDescriptor *BuiltinRegistry::find(std::string_view name) const noexcept {
    for (const auto &descriptor : table_)
        if (descriptor.name == name)
            return &descriptor;
    return nullptr;
}
const BuiltinDescriptor &BuiltinRegistry::precheck(std::string_view name, std::uint32_t count,
                                                   ErrorSite site) const {
    auto descriptor = find(name);
    if (!descriptor)
        fail(site, FailureDomain::Runtime, ReasonCode::Name, "unknown call target");
    if (descriptor->arity != count)
        fail(site, FailureDomain::Runtime, ReasonCode::Type, "wrong number of arguments");
    return *descriptor;
}
} // namespace myb
