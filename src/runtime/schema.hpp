#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace myb {
using SchemaId = std::uint32_t;
enum class TypeKind : std::uint8_t { Null, Bool, Int64, Float64, String, Array, Struct };
struct FieldType {
    TypeKind kind;
    std::optional<SchemaId> schema_id{};
    bool nullable{};
};
using FieldConstraint = FieldType;
struct FieldSchema {
    std::string name;
    FieldType type;
};
struct StructSchema {
    SchemaId id;
    std::string name;
    std::vector<FieldSchema> fields;
};
} // namespace myb
