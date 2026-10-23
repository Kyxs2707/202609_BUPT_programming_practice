#pragma once
#include <cassert>
#include <cstdint>
#include <type_traits>
#include <variant>

namespace myb {
struct StringObject;
struct ArrayObject;
struct StructObject;
/** @brief 表示 MYB 的 null 值，不承载对象引用。 */
struct NullValue {};
/**
 * @brief 借用 Heap 拥有的字符串对象，复制引用只复制对象地址。
 * @pre 构造时指针非空；使用期间对象须由同一会话的根保持可达。
 * 对象地址在回收前保持稳定；根撤销后的回收或 Heap 清空使引用失效。
 */
struct StringRef {
    StringObject *ptr;
    explicit StringRef(StringObject *p) noexcept : ptr(p) {
        assert(p);
    }
};
/**
 * @brief 借用 Heap 拥有的数组对象，多个引用共享元素修改。
 * @pre 构造时指针非空；使用期间对象须由同一会话的根保持可达。
 * Heap 回收或清空可使引用失效；数组扩容仅使元素地址失效。
 */
struct ArrayRef {
    ArrayObject *ptr;
    explicit ArrayRef(ArrayObject *p) noexcept : ptr(p) {
        assert(p);
    }
};
/**
 * @brief 借用 Heap 拥有的结构对象，复制引用保留对象身份。
 * @pre 构造时指针非空；使用期间对象须由同一会话的根保持可达。
 * 对象被回收或 Heap 清空后引用失效。
 */
struct StructRef {
    StructObject *ptr;
    explicit StructRef(StructObject *p) noexcept : ptr(p) {
        assert(p);
    }
};
/** @brief 保存七类 MYB 值的表示；对象引用不拥有对象，也不自行登记根。 */
using ValueData =
    std::variant<NullValue, bool, std::int64_t, double, StringRef, ArrayRef, StructRef>;
/**
 * @brief 以值表示复制标量或共享对象引用的运行时值。
 * 复制、替换和析构不触发 MYB GC；跨可回收调用保存对象值时须另行登记根。
 * @pre 写入的 double 必须有限；解析和运算入口负责检查，保留浮点正负零。
 */
struct Value {
    ValueData data;
};
/**
 * @brief 区分失效槽、已声明未初始化槽与可读槽。
 * 局部槽的根枚举只包含 Initialized 状态；槽内的默认 null 不代表已经初始化。
 * 操作数槽的有效性由帧的 sp 决定，不使用本状态判断。
 */
enum class BindingState : std::uint8_t { Dormant, Uninitialized, Initialized };
static_assert(std::is_nothrow_copy_constructible_v<Value>);
static_assert(std::is_nothrow_move_assignable_v<Value>);
} // namespace myb
