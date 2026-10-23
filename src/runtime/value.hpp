#pragma once
#include <cassert>
#include <cstdint>
#include <type_traits>
#include <variant>

namespace myb {
struct StringObject;
struct ArrayObject;
struct StructObject;
struct NullValue {};
struct StringRef {
    StringObject *ptr;
    explicit StringRef(StringObject *p) noexcept : ptr(p) {
        assert(p);
    }
};
struct ArrayRef {
    ArrayObject *ptr;
    explicit ArrayRef(ArrayObject *p) noexcept : ptr(p) {
        assert(p);
    }
};
struct StructRef {
    StructObject *ptr;
    explicit StructRef(StructObject *p) noexcept : ptr(p) {
        assert(p);
    }
};
using ValueData =
    std::variant<NullValue, bool, std::int64_t, double, StringRef, ArrayRef, StructRef>;
struct Value {
    ValueData data;
};
enum class BindingState : std::uint8_t { Dormant, Uninitialized, Initialized };
static_assert(std::is_nothrow_copy_constructible_v<Value>);
static_assert(std::is_nothrow_move_assignable_v<Value>);
} // namespace myb
