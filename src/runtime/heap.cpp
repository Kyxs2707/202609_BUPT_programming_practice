#include "runtime/heap.hpp"
#include "runtime/text_builder.hpp"
#include "source/utf8.hpp"
#include <algorithm>
#include <cassert>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>

namespace myb {
Heap::Heap(RootRegistry &roots, EmergencyFailure &failure) noexcept
    : roots_(roots), failure_(failure) {}
Heap::~Heap() noexcept {
    assert(builder_bytes_ == 0);
    clear();
}

[[noreturn]] void Heap::allocation_failed() {
    fail({failure_, {}}, FailureDomain::Resource, ReasonCode::Allocation,
         "managed heap allocation failed");
}
[[noreturn]] void Heap::capacity_failed() {
    fail({failure_, {}}, FailureDomain::Resource, ReasonCode::Capacity,
         "managed heap capacity exceeded");
}
[[noreturn]] void Heap::invariant_failed(const char *message) {
    fail({failure_, {}}, FailureDomain::Internal, ReasonCode::Invariant, message);
}
std::size_t Heap::add(std::size_t a, std::size_t b) {
    if (b > std::numeric_limits<std::size_t>::max() - a)
        capacity_failed();
    return a + b;
}
std::size_t Heap::multiply(std::size_t a, std::size_t b) {
    if (b && a > std::numeric_limits<std::size_t>::max() / b)
        capacity_failed();
    return a * b;
}
void Heap::check_elements(std::size_t required, std::size_t maximum) {
    if (required > maximum ||
        required > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        capacity_failed();
}
void Heap::check_output(const RootedValue &out) {
    if (!out.belongs_to(roots_))
        invariant_failed("output root does not belong to this heap session");
}
void Heap::update_threshold() noexcept {
    // All accounting changes checked this sum before committing.
    const auto total = managed_bytes_ + builder_bytes_;
    const auto maximum = std::numeric_limits<std::size_t>::max();
    next_gc_ = total > maximum / 2 ? maximum : std::max(minimum_gc, total * 2);
}
void Heap::before_growth(std::size_t growth) {
    if (collecting_)
        invariant_failed("managed allocation during heap collection");
    const auto expected = add(add(managed_bytes_, builder_bytes_), growth);
    large_request_ = false;
    bool force = false;
#ifdef MYB_TEST_HOOKS
    force = TestHooks::gc_forced();
#endif
    if (!failure_.is_set() && (force || expected > next_gc_))
        collect();
    // Only a request still large after this collection advances the threshold on success.
    large_request_ = growth >= next_gc_;
}
void Heap::after_growth() noexcept {
    if (large_request_)
        update_threshold();
    large_request_ = false;
}
std::size_t Heap::object_bytes(const HeapObject &object) {
    switch (object.kind) {
    case ObjectKind::Array:
        return add(
            sizeof(ArrayObject),
            multiply(static_cast<const ArrayObject &>(object).elements.capacity(), sizeof(Value)));
    case ObjectKind::String:
        return add(sizeof(StringObject), static_cast<const StringObject &>(object).utf8.capacity());
    case ObjectKind::Struct:
        return add(
            sizeof(StructObject),
            multiply(static_cast<const StructObject &>(object).fields.capacity(), sizeof(Value)));
    }
    invariant_failed("invalid managed object kind");
}
void Heap::update_builder(std::size_t old_capacity, std::size_t new_capacity) {
    assert(builder_bytes_ >= old_capacity);
    const auto updated = add(builder_bytes_ - old_capacity, new_capacity);
    (void)add(managed_bytes_, updated);
    builder_bytes_ = updated;
}

void Heap::make_array(std::size_t initial_capacity, RootedValue &out) {
    check_output(out);
    check_elements(initial_capacity, std::vector<Value>().max_size());
    before_growth(add(sizeof(ArrayObject), multiply(initial_capacity, sizeof(Value))));
    try {
#ifdef MYB_TEST_HOOKS
        TestHooks::hit(FaultPoint::HeapObject);
#endif
        auto object = std::make_unique<ArrayObject>();
        if (initial_capacity) {
#ifdef MYB_TEST_HOOKS
            TestHooks::hit(FaultPoint::Payload);
#endif
            object->elements.reserve(initial_capacity);
        }
        const auto updated = add(managed_bytes_, object_bytes(*object));
        (void)add(updated, builder_bytes_);
        auto *pointer = object.get();
        // The entire construction/registration/publication interval is GC-free.
        if (objects_.size() == objects_.capacity()) {
            if (objects_.size() == objects_.max_size())
                capacity_failed();
#ifdef MYB_TEST_HOOKS
            TestHooks::hit(FaultPoint::HeapRegistry);
#endif
        }
        objects_.push_back(std::move(object));
        managed_bytes_ = updated;
        out.set(Value{ArrayRef{pointer}});
        after_growth();
    } catch (const std::bad_alloc &) {
        allocation_failed();
    } catch (const std::length_error &) {
        capacity_failed();
    }
}
void Heap::make_struct(const StructSchema &schema, RootedValue &out) {
    check_output(out);
    check_elements(schema.fields.size(), std::vector<Value>().max_size());
    before_growth(add(sizeof(StructObject), multiply(schema.fields.size(), sizeof(Value))));
    try {
#ifdef MYB_TEST_HOOKS
        TestHooks::hit(FaultPoint::HeapObject);
#endif
        auto object = std::make_unique<StructObject>(schema);
        if (!schema.fields.empty()) {
#ifdef MYB_TEST_HOOKS
            TestHooks::hit(FaultPoint::Payload);
#endif
            object->fields.resize(schema.fields.size(), Value{NullValue{}});
        }
        const auto updated = add(managed_bytes_, object_bytes(*object));
        (void)add(updated, builder_bytes_);
        auto *pointer = object.get();
        if (objects_.size() == objects_.capacity()) {
            if (objects_.size() == objects_.max_size())
                capacity_failed();
#ifdef MYB_TEST_HOOKS
            TestHooks::hit(FaultPoint::HeapRegistry);
#endif
        }
        objects_.push_back(std::move(object));
        managed_bytes_ = updated;
        out.set(Value{StructRef{pointer}});
        after_growth();
    } catch (const std::bad_alloc &) {
        allocation_failed();
    } catch (const std::length_error &) {
        capacity_failed();
    }
}
void Heap::make_string(TextBuilder &&builder, RootedValue &out) {
    check_output(out);
    if (&builder.heap_ != this || builder.consumed_)
        invariant_failed("invalid text builder transfer");
    const auto count = utf8::count_code_points(builder.bytes());
    if (!count)
        invariant_failed("string publication requires valid UTF-8");
    check_elements(*count, std::numeric_limits<std::size_t>::max());
    // The already-accounted builder payload is transferred, rather than allocated again.
    before_growth(sizeof(StringObject));
    try {
#ifdef MYB_TEST_HOOKS
        TestHooks::hit(FaultPoint::HeapObject);
#endif
        auto object = std::make_unique<StringObject>();
        (void)add(add(managed_bytes_, sizeof(StringObject)),
                  add(builder_bytes_ - builder.accounted_capacity_,
                      std::max(builder.buffer_.capacity(), object->utf8.capacity())));
        if (objects_.size() == objects_.capacity()) {
            if (objects_.size() == objects_.max_size())
                capacity_failed();
#ifdef MYB_TEST_HOOKS
            TestHooks::hit(FaultPoint::HeapRegistry);
#endif
            const auto old_capacity = objects_.capacity();
            const auto step = std::max<std::size_t>(old_capacity / 2, 1);
            const auto capacity = step > objects_.max_size() - old_capacity ? objects_.max_size()
                                                                            : old_capacity + step;
            objects_.reserve(capacity);
        }
        // std::string with the default allocator transfers without allocation or GC.
        object->utf8 = std::move(builder.buffer_);
        object->codepoint_count = *count;
        const auto updated = add(managed_bytes_, object_bytes(*object));
        const auto builders_updated = builder_bytes_ - builder.accounted_capacity_;
        (void)add(updated, builders_updated);
        auto *pointer = object.get();
        objects_.push_back(std::move(object));
        managed_bytes_ = updated;
        builder_bytes_ = builders_updated;
        builder.accounted_capacity_ = 0;
        builder.consumed_ = true;
        out.set(Value{StringRef{pointer}});
        after_growth();
    } catch (const std::bad_alloc &) {
        allocation_failed();
    } catch (const std::length_error &) {
        capacity_failed();
    }
}
void Heap::reserve_array(ArrayRef array, std::size_t required_capacity) {
    auto &elements = array.ptr->elements;
    check_elements(required_capacity, elements.max_size());
    if (required_capacity <= elements.capacity())
        return;
    const auto previous = multiply(elements.capacity(), sizeof(Value));
    const auto requested = multiply(required_capacity, sizeof(Value));
    before_growth(requested - previous);
    try {
#ifdef MYB_TEST_HOOKS
        TestHooks::hit(FaultPoint::Payload);
#endif
        elements.reserve(required_capacity);
        const auto updated =
            add(managed_bytes_ - previous, multiply(elements.capacity(), sizeof(Value)));
        (void)add(updated, builder_bytes_);
        managed_bytes_ = updated;
        after_growth();
    } catch (const std::bad_alloc &) {
        allocation_failed();
    } catch (const std::length_error &) {
        capacity_failed();
    }
}

void Marker::mark(Value value) {
    std::visit(
        [this](auto item) {
            using T = decltype(item);
            if constexpr (std::is_same_v<T, StringRef> || std::is_same_v<T, ArrayRef> ||
                          std::is_same_v<T, StructRef>) {
                if (!item.ptr)
                    heap_.invariant_failed("null managed reference");
                mark_object(item.ptr);
            }
        },
        value.data);
}
void Marker::mark_object(HeapObject *object) {
    if (object->marked)
        return;
    object->marked = true;
    if (heap_.mark_worklist_.size() == heap_.mark_worklist_.capacity()) {
        if (heap_.mark_worklist_.size() == heap_.mark_worklist_.max_size())
            heap_.capacity_failed();
#ifdef MYB_TEST_HOOKS
        TestHooks::hit(FaultPoint::MarkWorklist);
#endif
    }
    heap_.mark_worklist_.push_back(object);
}
void Heap::collect() {
    if (collecting_)
        invariant_failed("recursive heap collection");
    collecting_ = true;
    try {
        try {
            Marker marker(*this);
            roots_.trace(marker);
            while (!mark_worklist_.empty()) {
                auto *object = mark_worklist_.back();
                mark_worklist_.pop_back();
                switch (object->kind) {
                case ObjectKind::String:
                    break;
                case ObjectKind::Array:
                    for (const auto &value : static_cast<ArrayObject *>(object)->elements)
                        marker.mark(value);
                    break;
                case ObjectKind::Struct:
                    for (const auto &value : static_cast<StructObject *>(object)->fields)
                        marker.mark(value);
                    break;
                }
            }
            // Validate survivor accounting before entering the nonthrowing sweep.
            std::size_t survivor_bytes = 0;
            for (const auto &object : objects_)
                if (object->marked)
                    survivor_bytes = add(survivor_bytes, object_bytes(*object));
            (void)add(survivor_bytes, builder_bytes_);
            std::size_t destination = 0;
            for (std::size_t source = 0; source < objects_.size(); ++source) {
                if (objects_[source]->marked) {
                    objects_[source]->marked = false;
                    if (destination != source)
                        objects_[destination] = std::move(objects_[source]);
                    ++destination;
                }
            }
            objects_.resize(destination);
            managed_bytes_ = survivor_bytes;
            update_threshold();
            collecting_ = false;
        } catch (...) {
            for (const auto &object : objects_)
                if (object)
                    object->marked = false;
            mark_worklist_.clear();
            collecting_ = false;
            throw;
        }
    } catch (const std::bad_alloc &) {
        allocation_failed();
    } catch (const std::length_error &) {
        capacity_failed();
    }
}
void Heap::clear() noexcept {
    assert(!collecting_);
    assert(builder_bytes_ == 0);
    objects_.clear();
    mark_worklist_.clear();
    managed_bytes_ = 0;
    large_request_ = false;
    update_threshold();
}
#ifdef MYB_TEST_HOOKS
TestHooks::HeapCounts TestHooks::heap_counts(const Heap &heap) noexcept {
    HeapCounts counts{heap.objects_.size(), 0, heap.collecting_, 0};
    for (const auto &object : heap.objects_) {
        if (object->marked)
            ++counts.marked;
        if (object->kind == ObjectKind::Array &&
            static_cast<const ArrayObject &>(*object).active_iteration_count)
            ++counts.active_arrays;
    }
    return counts;
}
#endif
} // namespace myb
