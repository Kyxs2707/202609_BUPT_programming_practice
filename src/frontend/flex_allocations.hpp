#pragma once
#include "diagnostic/failure.hpp"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <limits>
#ifdef MYB_TEST_HOOKS
#include "test_hooks.hpp"
#endif
namespace myb {
class FlexAllocationLedger {
    struct alignas(std::max_align_t) Block {
        Block *prev;
        Block *next;
        std::size_t size;
    };
    Block *head_{};
    inline static std::size_t live_{};
    EmergencyFailure &failure_;

  public:
    explicit FlexAllocationLedger(EmergencyFailure &f) : failure_(f) {}
    ~FlexAllocationLedger() noexcept {
        clear();
    }
    FlexAllocationLedger(const FlexAllocationLedger &) = delete;
    void *allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() - sizeof(Block))
            fail({failure_, {}}, FailureDomain::Resource, ReasonCode::Capacity,
                 "scanner allocation exceeds capacity");
#ifdef MYB_TEST_HOOKS
        try {
            TestHooks::hit(FaultPoint::FlexAllocation);
        } catch (const std::bad_alloc &) {
            fail({failure_, {}}, FailureDomain::Resource, ReasonCode::Allocation,
                 "scanner allocation failed");
        }
#endif
        auto *b = static_cast<Block *>(std::malloc(sizeof(Block) + n));
        if (!b)
            fail({failure_, {}}, FailureDomain::Resource, ReasonCode::Allocation,
                 "scanner allocation failed");
        *b = {nullptr, head_, n};
        if (head_)
            head_->prev = b;
        head_ = b;
        ++live_;
        return b + 1;
    }
    void release(void *p) noexcept {
        if (!p)
            return;
        auto *b = static_cast<Block *>(p) - 1;
        if (b->prev)
            b->prev->next = b->next;
        else
            head_ = b->next;
        if (b->next)
            b->next->prev = b->prev;
        std::free(b);
        --live_;
    }
    void *reallocate(void *p, std::size_t n) {
        if (!p)
            return allocate(n);
        auto *old = static_cast<Block *>(p) - 1;
        auto *fresh = allocate(n);
        std::memcpy(fresh, p, std::min(old->size, n));
        release(p);
        return fresh;
    }
    void clear() noexcept {
        while (head_)
            release(head_ + 1);
    }
    static std::size_t live_total() noexcept {
        return live_;
    }
};
} // namespace myb
