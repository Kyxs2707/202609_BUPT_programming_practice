#include "builtins/registry.hpp"
#include "runtime/display.hpp"
#include "runtime/text_builder.hpp"
namespace myb {
namespace {
void output(NativeContext &context, std::span<const Value> args, RootedValue &result,
            bool newline) {
    TextBuilder text(context.heap);
    display(args[0], text, {context.failure, context.span});
    context.io.write_stdout(text.bytes(), newline);
    result.set(Value{NullValue{}});
}
void print_value(NativeContext &context, std::span<const Value> args, RootedValue &result) {
    output(context, args, result, true);
}
void write_value(NativeContext &context, std::span<const Value> args, RootedValue &result) {
    output(context, args, result, false);
}
constexpr BuiltinDescriptor descriptors[] = {{"print", 1, print_value}, {"write", 1, write_value}};
} // namespace
std::span<const BuiltinDescriptor> builtin_descriptors() noexcept {
    return descriptors;
}
} // namespace myb
