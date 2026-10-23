#pragma once
#include "runtime/schema.hpp"
#include "runtime/value.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace myb {
enum class ObjectKind : std::uint8_t { String, Array, Struct };
struct HeapObject {
    explicit HeapObject(ObjectKind k) noexcept : kind(k) {}
    virtual ~HeapObject() noexcept = default;
    ObjectKind kind;
    bool marked{};
};
struct StringObject final : HeapObject {
    StringObject() noexcept : HeapObject(ObjectKind::String) {}
    std::string utf8;
    std::size_t codepoint_count{};
};
struct ArrayObject final : HeapObject {
    ArrayObject() noexcept : HeapObject(ObjectKind::Array) {}
    std::vector<Value> elements;
    std::size_t active_iteration_count{};
};
struct StructObject final : HeapObject {
    explicit StructObject(const StructSchema &s) noexcept
        : HeapObject(ObjectKind::Struct), schema(&s) {}
    const StructSchema *schema;
    std::vector<Value> fields;
};
} // namespace myb
