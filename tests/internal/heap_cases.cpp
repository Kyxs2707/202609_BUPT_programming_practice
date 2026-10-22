#include "diagnostic/failure.hpp"
#include "runtime/heap.hpp"
#include "runtime/text_builder.hpp"
#include "test_hooks.hpp"
#include "test_support.hpp"
#include <limits>
#include <type_traits>
#include <vector>

using namespace myb;

namespace {
ArrayRef array(const RootedValue &root) {
    return std::get<ArrayRef>(root.get().data);
}
void trace_values(void *context, Marker &marker) {
    for (const auto &value : *static_cast<std::vector<Value> *>(context))
        marker.mark(value);
}
template <class Action> void resource_failure(Action &&action, EmergencyFailure &first) {
    bool threw = false;
    try {
        action();
    } catch (const Failure &failure) {
        threw = true;
        REQUIRE(failure.header().domain == FailureDomain::Resource);
    }
    REQUIRE(threw);
    REQUIRE(first.is_set());
}
} // namespace

// Removing graph traversal, tracing stale vector slots, or treating registration as a root breaks
// these assertions.
MYB_TEST(heap_roots) {
    static_assert(!std::is_copy_constructible_v<RootedValue>);
    static_assert(!std::is_move_constructible_v<NativeRootSet>);
    EmergencyFailure first;
    RootRegistry roots;
    Heap heap(roots, first);
    {
        RootedValue a(roots);
        heap.make_array(1, a);
        REQUIRE(TestHooks::heap_counts(heap).live == 1);
        auto self = array(a);
        self.ptr->elements.push_back(a.get());
        {
            RootedValue alias(roots, a.get());
            a.set(Value{NullValue{}});
            heap.collect();
            REQUIRE(TestHooks::heap_counts(heap).live == 1);
            REQUIRE(std::get<ArrayRef>(self.ptr->elements[0].data).ptr == self.ptr);
        }
        heap.collect();
        REQUIRE(TestHooks::heap_counts(heap).live == 0);
    }
    {
        std::vector<Value> constants;
        NativeRootSet provider(roots, &constants, trace_values);
        for (int i = 0; i < 160; ++i) {
            RootedValue result(roots);
            heap.make_array(0, result);
            constants.push_back(result.get());
        }
        heap.collect();
        REQUIRE(TestHooks::heap_counts(heap).live == 160);
        constants.clear();
        heap.collect();
        REQUIRE(TestHooks::heap_counts(heap).live == 0);
        provider.detach();
        provider.detach();
    }
    StructSchema schema{7, "Node", {{"next", {TypeKind::Struct, 7, true}}}};
    {
        RootedValue structure(roots), child(roots);
        heap.make_struct(schema, structure);
        heap.make_array(0, child);
        auto ref = std::get<StructRef>(structure.get().data);
        ref.ptr->fields[0] = child.get();
        child.set(Value{NullValue{}});
        heap.collect();
        REQUIRE(TestHooks::heap_counts(heap).live == 2);
        REQUIRE(ref.ptr->schema == &schema);
        REQUIRE(ref.ptr->fields.size() == 1);
    }
    heap.clear();
    REQUIRE(TestHooks::heap_counts(heap).live == 0);
}

// Missing temporary roots, publication gaps, byte-length accounting, or partial-mark sweep break
// these assertions.
MYB_TEST(heap_growth) {
    EmergencyFailure first;
    RootRegistry roots;
    Heap heap(roots, first);
    TestHooks::force_gc(true);
    {
        RootedValue parent(roots), child(roots), text(roots);
        heap.make_array(1, parent);
        REQUIRE(TestHooks::heap_counts(heap).live == 1);
        auto parent_ref = array(parent);
        heap.make_array(0, child);
        auto child_ref = array(child);
        parent_ref.ptr->elements.push_back(child.get());
        child.set(Value{NullValue{}});
        heap.reserve_array(parent_ref, 400);
        REQUIRE(array(parent).ptr == parent_ref.ptr);
        REQUIRE(std::get<ArrayRef>(parent_ref.ptr->elements[0].data).ptr == child_ref.ptr);
        const std::string input("\xe5\x8c\x97\xe9\x82\xae\xf0\x9f\x98\x80\0", 11);
        TextBuilder builder(heap);
        builder.append(std::span<const char>(input.data(), 5));
        builder.append(std::span<const char>(input.data() + 5, input.size() - 5));
        builder.reserve(512);
        heap.make_string(std::move(builder), text);
        auto string_ref = std::get<StringRef>(text.get().data);
        REQUIRE(string_ref.ptr->utf8 == input);
        REQUIRE(string_ref.ptr->codepoint_count == 4);
        heap.collect();
        REQUIRE(TestHooks::heap_counts(heap).live == 3);
        // A failure before replacement must preserve the existing output root.
        TestHooks::fail_next(FaultPoint::HeapObject);
        resource_failure([&] { heap.make_array(0, text); }, first);
        REQUIRE(std::get<StringRef>(text.get().data).ptr == string_ref.ptr);
        REQUIRE(TestHooks::heap_counts(heap).live == 3);
    }
    TestHooks::force_gc(false);
    heap.clear();
    for (auto point : {FaultPoint::HeapObject, FaultPoint::HeapRegistry, FaultPoint::Payload}) {
        EmergencyFailure fault_first;
        RootRegistry fault_roots;
        Heap fault_heap(fault_roots, fault_first);
        RootedValue out(fault_roots);
        TestHooks::fail_next(point);
        resource_failure([&] { fault_heap.make_array(32, out); }, fault_first);
        REQUIRE(TestHooks::heap_counts(fault_heap).live == 0);
        REQUIRE(std::holds_alternative<NullValue>(out.get().data));
    }
    {
        EmergencyFailure fault_first;
        RootRegistry fault_roots;
        Heap fault_heap(fault_roots, fault_first);
        RootedValue out(fault_roots);
        fault_heap.make_array(1, out);
        array(out).ptr->elements.push_back(out.get());
        {
            RootedValue unreachable(fault_roots);
            fault_heap.make_array(0, unreachable);
        }
        TestHooks::fail_next(FaultPoint::MarkWorklist);
        resource_failure([&] { fault_heap.collect(); }, fault_first);
        const auto counts = TestHooks::heap_counts(fault_heap);
        REQUIRE(counts.live ==
                2); // The unmarked unreachable object must not be swept after incomplete marking.
        REQUIRE(counts.marked == 0);
        REQUIRE(!counts.collecting);
        fault_heap.clear();
        REQUIRE(TestHooks::heap_counts(fault_heap).live == 0);
    }
    {
        EmergencyFailure capacity_first;
        RootRegistry capacity_roots;
        Heap capacity_heap(capacity_roots, capacity_first);
        RootedValue out(capacity_roots);
        resource_failure(
            [&] { capacity_heap.make_array(std::numeric_limits<std::size_t>::max(), out); },
            capacity_first);
        REQUIRE(capacity_first.header().reason == ReasonCode::Capacity);
    }
    TestHooks::clear_fault();
}

// Builder memory participates in pressure before publication; transfers must neither double charge
// nor release live bytes.
MYB_TEST(heap_builder_pressure) {
    EmergencyFailure first;
    RootRegistry roots;
    Heap heap(roots, first);
    RootedValue sentinel(roots), text(roots);
    heap.make_array(0, sentinel);
    sentinel.set(Value{NullValue{}});
    {
        TextBuilder builder(heap);
        builder.reserve(2 * 1024 * 1024);
        REQUIRE(TestHooks::heap_counts(heap).live == 0);
        std::string content(2 * 1024 * 1024, 'x');
        builder.append(std::span<const char>(content.data(), content.size()));
        heap.make_string(std::move(builder), text);
    }
    // The large request establishes headroom, and transferring its payload keeps that headroom
    // intact.
    heap.make_array(0, sentinel);
    sentinel.set(Value{NullValue{}});
    {
        RootedValue next(roots);
        heap.make_array(0, next);
        REQUIRE(TestHooks::heap_counts(heap).live == 3);
    }
    heap.collect();
    REQUIRE(TestHooks::heap_counts(heap).live == 1);
    text.set(Value{NullValue{}});
    heap.collect();
    REQUIRE(TestHooks::heap_counts(heap).live == 0);
    // Destruction returns builder pressure; it cannot leave a phantom large live allocation.
    {
        TextBuilder transient(heap);
        transient.reserve(3 * 1024 * 1024);
    }
    heap.collect();
    heap.make_array(0, sentinel);
    sentinel.set(Value{NullValue{}});
    {
        TextBuilder next(heap);
        next.reserve(2 * 1024 * 1024);
        REQUIRE(TestHooks::heap_counts(heap).live == 0);
    }
}

// Rejecting or partially applying growth must preserve the previous text/array and the old output
// root.
MYB_TEST(heap_failed_growth) {
    for (bool use_builder : {false, true}) {
        EmergencyFailure first;
        RootRegistry roots;
        Heap heap(roots, first);
        RootedValue out(roots);
        heap.make_array(1, out);
        auto old = array(out);
        old.ptr->elements.push_back(Value{std::int64_t{42}});
        TextBuilder builder(heap);
        const char bytes[] = "abc";
        builder.append(std::span<const char>(bytes, 3));
        TestHooks::fail_next(FaultPoint::Payload);
        if (use_builder)
            resource_failure([&] { builder.reserve(512); }, first);
        else
            resource_failure([&] { heap.reserve_array(old, 512); }, first);
        REQUIRE(old.ptr == array(out).ptr);
        REQUIRE(old.ptr->elements.size() == 1);
        REQUIRE(std::get<std::int64_t>(old.ptr->elements[0].data) == 42);
        REQUIRE(builder.bytes() == "abc");
        REQUIRE(TestHooks::heap_counts(heap).live == 1);
    }
    for (auto point : {FaultPoint::HeapObject, FaultPoint::HeapRegistry}) {
        EmergencyFailure first;
        RootRegistry roots;
        Heap heap(roots, first);
        RootedValue out(roots);
        TextBuilder builder(heap);
        const std::string content(512, 'x');
        builder.append(std::span<const char>(content.data(), content.size()));
        TestHooks::fail_next(point);
        resource_failure([&] { heap.make_string(std::move(builder), out); }, first);
        REQUIRE(builder.bytes() == content);
        REQUIRE(TestHooks::heap_counts(heap).live == 0);
        REQUIRE(std::holds_alternative<NullValue>(out.get().data));
    }
    for (auto point : {FaultPoint::HeapObject, FaultPoint::HeapRegistry, FaultPoint::Payload}) {
        EmergencyFailure first;
        RootRegistry roots;
        StructSchema schema{
            9, "Pair", {{"x", {TypeKind::Int64, {}, false}}, {"y", {TypeKind::String, {}, true}}}};
        Heap heap(roots, first);
        RootedValue out(roots);
        TestHooks::fail_next(point);
        resource_failure([&] { heap.make_struct(schema, out); }, first);
        REQUIRE(TestHooks::heap_counts(heap).live == 0);
        REQUIRE(std::holds_alternative<NullValue>(out.get().data));
    }
    TestHooks::clear_fault();
}

// A deep object graph must not recurse through the C++ stack during tracing or destruction.
MYB_TEST(heap_deep_graph) {
    EmergencyFailure first;
    RootRegistry roots;
    Heap heap(roots, first);
    {
        RootedValue head(roots), tail(roots), next(roots);
        heap.make_array(1, head);
        tail.set(head.get());
        for (int i = 1; i < 20000; ++i) {
            heap.make_array(1, next);
            array(tail).ptr->elements.push_back(next.get());
            tail.set(next.get());
        }
        next.set(Value{NullValue{}});
        tail.set(Value{NullValue{}});
        heap.collect();
        REQUIRE(TestHooks::heap_counts(heap).live == 20000);
    }
    heap.collect();
    REQUIRE(TestHooks::heap_counts(heap).live == 0);
}
