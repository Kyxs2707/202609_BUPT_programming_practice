#include "runtime/roots.hpp"
#include "runtime/heap.hpp"
#include <cassert>

namespace myb {
RootRegistry::~RootRegistry() noexcept {
    assert(!head_);
}
void RootRegistry::trace(Marker &marker) {
    for (auto *p = head_; p; p = p->next_)
        p->trace_(p->context_, marker);
}
RootLink::RootLink(RootRegistry &registry, void *context, RootTrace trace) noexcept
    : registry_(&registry), next_(registry.head_), context_(context), trace_(trace) {
    assert(context && trace);
    if (next_)
        next_->previous_ = this;
    registry.head_ = this;
}
RootLink::~RootLink() noexcept {
    detach();
}
void RootLink::detach() noexcept {
    if (!registry_)
        return;
    if (previous_)
        previous_->next_ = next_;
    else
        registry_->head_ = next_;
    if (next_)
        next_->previous_ = previous_;
    registry_ = nullptr;
    previous_ = nullptr;
    next_ = nullptr;
}
bool RootLink::belongs_to(const RootRegistry &registry) const noexcept {
    return registry_ == &registry;
}
RootedValue::RootedValue(RootRegistry &registry) noexcept : link_(registry, this, trace_slot) {}
RootedValue::RootedValue(RootRegistry &registry, Value value) noexcept
    : slot_(value), link_(registry, this, trace_slot) {}
void RootedValue::trace_slot(void *context, Marker &marker) {
    marker.mark(static_cast<RootedValue *>(context)->slot_);
}
} // namespace myb
