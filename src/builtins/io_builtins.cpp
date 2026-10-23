#include "builtins/registry.hpp"
#include "runtime/display.hpp"
#include "runtime/text_builder.hpp"
namespace myb {
namespace {
/**
 * @brief 显示单个参数并写入标准输出，成功时将 null 写入输出根。
 * @pre args 含一个由 VM 有效操作数区保活的值，result 已登记于 context.roots。
 *
 * 文本缓冲增长可能触发 GC，参数根须覆盖显示及写入；宿主写入失败直接传播异常。
 */
void output(NativeContext &context, std::span<const Value> args, RootedValue &result,
            bool newline) {
    TextBuilder text(context.heap);
    display(args[0], text, {context.failure, context.span});
    context.io.write_stdout(text.bytes(), newline);
    result.set(Value{NullValue{}});
}
/** @brief 使用内置回调协议显示单个值，并在成功写入后追加换行。 */
void print_value(NativeContext &context, std::span<const Value> args, RootedValue &result) {
    output(context, args, result, true);
}
/** @brief 使用内置回调协议显示单个值，保留其文本原有的换行。 */
void write_value(NativeContext &context, std::span<const Value> args, RootedValue &result) {
    output(context, args, result, false);
}
constexpr BuiltinDescriptor descriptors[] = {{"print", 1, print_value}, {"write", 1, write_value}};
} // namespace
std::span<const BuiltinDescriptor> builtin_descriptors() noexcept {
    return descriptors;
}
} // namespace myb
