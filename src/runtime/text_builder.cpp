#include "runtime/text_builder.hpp"
#include "runtime/heap.hpp"
#include <cassert>
#include <new>
#include <stdexcept>

namespace myb {
TextBuilder::TextBuilder(Heap &heap) : heap_(heap) {
    heap_.update_builder(0, buffer_.capacity());
    accounted_capacity_ = buffer_.capacity();
}
TextBuilder::~TextBuilder() noexcept {
    assert(heap_.builder_bytes_ >= accounted_capacity_);
    heap_.builder_bytes_ -= accounted_capacity_;
}
void TextBuilder::reserve(std::size_t capacity) {
    if (consumed_)
        heap_.invariant_failed("text builder used after transfer");
    if (capacity > buffer_.max_size())
        heap_.capacity_failed();
    if (capacity <= buffer_.capacity())
        return;
    heap_.before_growth(capacity - buffer_.capacity());
    try {
#ifdef MYB_TEST_HOOKS
        TestHooks::hit(FaultPoint::Payload);
#endif
        buffer_.reserve(capacity);
        heap_.update_builder(accounted_capacity_, buffer_.capacity());
        accounted_capacity_ = buffer_.capacity();
        heap_.after_growth();
    } catch (const std::bad_alloc &) {
        heap_.allocation_failed();
    } catch (const std::length_error &) {
        heap_.capacity_failed();
    }
}
void TextBuilder::append(std::span<const char> bytes) {
    if (consumed_)
        heap_.invariant_failed("text builder used after transfer");
    if (bytes.empty())
        return;
    const auto needed = heap_.add(buffer_.size(), bytes.size());
    if (needed > buffer_.max_size())
        heap_.capacity_failed();
    const bool grows = needed > buffer_.capacity();
    if (grows)
        heap_.before_growth(needed - buffer_.capacity());
    try {
#ifdef MYB_TEST_HOOKS
        if (grows)
            TestHooks::hit(FaultPoint::Payload);
#endif
        // string::append handles input that aliases its own current byte storage.
        buffer_.append(bytes.data(), bytes.size());
        if (grows) {
            heap_.update_builder(accounted_capacity_, buffer_.capacity());
            accounted_capacity_ = buffer_.capacity();
            heap_.after_growth();
        }
    } catch (const std::bad_alloc &) {
        heap_.allocation_failed();
    } catch (const std::length_error &) {
        heap_.capacity_failed();
    }
}
} // namespace myb
