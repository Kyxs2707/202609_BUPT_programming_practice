#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace myb {
/** @brief 在所属编译程序内标识结构定义的稳定编号。 */
using SchemaId = std::uint32_t;
/** @brief 描述结构字段可接受的运行时值类别。 */
enum class TypeKind : std::uint8_t { Null, Bool, Int64, Float64, String, Array, Struct };
/**
 * @brief 保存字段类型、结构身份和可空约束，供运行时字段检查使用。
 * Struct 类别的 schema_id 标识所属程序中的结构；nullable 表示额外接受 null。
 */
struct FieldType {
    TypeKind kind;
    std::optional<SchemaId> schema_id{};
    bool nullable{};
};
using FieldConstraint = FieldType;
/** @brief 拥有字段名称及类型约束，位置对应结构对象中的同序字段槽。 */
struct FieldSchema {
    std::string name;
    FieldType type;
};
/**
 * @brief 拥有结构名称和按声明顺序保存的字段定义。
 * 编译程序负责保持本对象地址稳定，并使其寿命覆盖所有借用它的 StructObject。
 */
struct StructSchema {
    SchemaId id;
    std::string name;
    std::vector<FieldSchema> fields;
};
} // namespace myb
