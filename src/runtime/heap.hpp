#pragma once
#include "diagnostic/failure.hpp"
#include "runtime/object.hpp"
#include "runtime/roots.hpp"
#include <memory>
#include <vector>
#ifdef MYB_TEST_HOOKS
#include "test_hooks.hpp"
#endif

namespace myb {
class TextBuilder;
class Heap;
class Marker final {
  public:
    void mark(Value);

  private:
    friend class Heap;
    explicit Marker(Heap &heap) noexcept : heap_(heap) {}
    void mark_object(HeapObject *);
    Heap &heap_;
};
class Heap final {
  public:
    Heap(RootRegistry &, EmergencyFailure &) noexcept;
    ~Heap() noexcept;
    Heap(const Heap &) = delete;
    Heap &operator=(const Heap &) = delete;
    void make_array(std::size_t initial_capacity, RootedValue &out);
    void make_struct(const StructSchema &, RootedValue &out);
    void make_string(TextBuilder &&, RootedValue &out);
    void reserve_array(ArrayRef, std::size_t required_capacity);
    void collect();
    void clear() noexcept;

  private:
    friend class Marker;
    friend class TextBuilder;
#ifdef MYB_TEST_HOOKS
    friend TestHooks::HeapCounts TestHooks::heap_counts(const Heap &) noexcept;
#endif
    std::size_t add(std::size_t, std::size_t);
    std::size_t multiply(std::size_t, std::size_t);
    void check_elements(std::size_t, std::size_t max_size);
    void check_output(const RootedValue &);
    void before_growth(std::size_t);
    void after_growth() noexcept;
    void update_threshold() noexcept;
    std::size_t object_bytes(const HeapObject &);
    void update_builder(std::size_t old_capacity, std::size_t new_capacity);
    [[noreturn]] void allocation_failed();
    [[noreturn]] void capacity_failed();
    [[noreturn]] void invariant_failed(const char *);
    RootRegistry &roots_;
    EmergencyFailure &failure_;
    std::vector<std::unique_ptr<HeapObject>> objects_;
    std::vector<HeapObject *> mark_worklist_;
    std::size_t managed_bytes_{};
    std::size_t builder_bytes_{};
    static constexpr std::size_t minimum_gc = 1024 * 1024;
    std::size_t next_gc_{minimum_gc};
    bool collecting_{};
    bool large_request_{};
};
} // namespace myb
