#pragma once
#include "runtime/schema.hpp"
#include "runtime/value.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace myb {
/** @brief 标明受管对象的具体负载，供标记和内存记账分派。 */
enum class ObjectKind : std::uint8_t { String, Array, Struct };
/**
 * @brief 由 Heap 独占并以非移动标记清除方式管理的对象基类。
 * 对象析构不追踪子引用；登记对象的 unique_ptr 移动时对象地址保持稳定。
 */
struct HeapObject {
    explicit HeapObject(ObjectKind k) noexcept : kind(k) {}
    virtual ~HeapObject() noexcept = default;
    ObjectKind kind; ///< 构造时确定的负载类别。
    bool marked{}; ///< 本轮标记是否已发现该对象，由 Heap 在回收结束或失败时复原。
};
/** @brief 保存已验证 UTF-8 及码点数的字符串；发布后按 MYB 字符串语义只读。 */
struct StringObject final : HeapObject {
    StringObject() noexcept : HeapObject(ObjectKind::String) {}
    std::string utf8;
    std::size_t codepoint_count{}; ///< Unicode 标量数量，由发布入口计算。
};
/**
 * @brief 保存可混合类型的共享数组负载及活动遍历计数。
 * elements 的 Value 不独占子对象，GC 按元素追踪引用；容量增长使元素借用失效。
 * 活动遍历期间的增删限制由执行数组操作的调用方检查。
 */
struct ArrayObject final : HeapObject {
    ArrayObject() noexcept : HeapObject(ObjectKind::Array) {}
    std::vector<Value> elements;
    std::size_t active_iteration_count{}; ///< 尚未结束的数组遍历数。
};
/**
 * @brief 按 Schema 声明顺序保存结构字段，并借用稳定的结构定义。
 * @pre schema 的地址和字段定义在对象存活期间保持有效；先销毁对象再销毁 Schema。
 * 初建字段可为内部 null 占位，逐字段类型校验由构造调用方负责。
 */
struct StructObject final : HeapObject {
    explicit StructObject(const StructSchema &s) noexcept
        : HeapObject(ObjectKind::Struct), schema(&s) {}
    const StructSchema *schema; ///< 非拥有的结构定义借用，不属于受管对象边。
    std::vector<Value> fields;
};
} // namespace myb
