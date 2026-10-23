#pragma once
#include "platform/host_io.hpp"
#include "runtime/heap.hpp"
#include <cstdint>
#include <span>
#include <string_view>
namespace myb {
struct NativeContext {
    Heap &heap;
    RootRegistry &roots;
    HostIO &io;
    EmergencyFailure &failure;
    SourceSpan span;
};
using NativeFn = void (*)(NativeContext &, std::span<const Value>, RootedValue &);
struct BuiltinDescriptor {
    std::string_view name;
    std::uint8_t arity;
    NativeFn invoke;
};
std::span<const BuiltinDescriptor> builtin_descriptors() noexcept;
class BuiltinRegistry final {
  public:
    explicit BuiltinRegistry(
        std::span<const BuiltinDescriptor> table = builtin_descriptors()) noexcept
        : table_(table) {}
    const BuiltinDescriptor *find(std::string_view) const noexcept;
    const BuiltinDescriptor &precheck(std::string_view, std::uint32_t, ErrorSite) const;

  private:
    std::span<const BuiltinDescriptor> table_;
};
} // namespace myb
