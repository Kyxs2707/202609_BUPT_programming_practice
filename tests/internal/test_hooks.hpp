#pragma once
#ifndef MYB_TEST_HOOKS
#error "Test hooks must only be included by internal test builds"
#endif
#include <cstddef>
#include <new>

namespace myb {
class Heap;
class VM;
enum class FaultPoint {
    HeapObject,
    HeapRegistry,
    Payload,
    MarkWorklist,
    ProgramConstants,
    NativeWorkspace,
    VmValues,
    VmStates,
    VmFrames,
    IterationRecords,
    FlexAllocation,
    DiagnosticSnapshot,
    OsRead,
    OsWrite,
    OsClose
};
namespace TestHooks {
struct HeapCounts {
    std::size_t live{};
    std::size_t marked{};
    bool collecting{};
    std::size_t active_arrays{};
};
void force_gc(bool enabled) noexcept;
bool gc_forced() noexcept;
void fail_next(FaultPoint point) noexcept;
void clear_fault() noexcept;
void hit(FaultPoint point);
HeapCounts heap_counts(const Heap &heap) noexcept;
std::size_t active_iteration_count(const VM &vm) noexcept;
} // namespace TestHooks
} // namespace myb
