#pragma once
#include "runtime/value.hpp"

namespace myb {
class Marker;
class RootLink;
using RootTrace = void (*)(void *, Marker &);
class RootRegistry final {
  public:
    RootRegistry() noexcept = default;
    ~RootRegistry() noexcept;
    RootRegistry(const RootRegistry &) = delete;
    RootRegistry &operator=(const RootRegistry &) = delete;
    void trace(Marker &);

  private:
    friend class RootLink;
    RootLink *head_{};
};
class RootLink final {
  public:
    RootLink(RootRegistry &, void *, RootTrace) noexcept;
    ~RootLink() noexcept;
    RootLink(const RootLink &) = delete;
    RootLink &operator=(const RootLink &) = delete;
    void detach() noexcept;
    bool belongs_to(const RootRegistry &) const noexcept;

  private:
    friend class RootRegistry;
    RootRegistry *registry_{};
    RootLink *previous_{};
    RootLink *next_{};
    void *context_;
    RootTrace trace_;
};
class RootedValue final {
  public:
    explicit RootedValue(RootRegistry &) noexcept;
    RootedValue(RootRegistry &, Value) noexcept;
    ~RootedValue() noexcept = default;
    RootedValue(const RootedValue &) = delete;
    RootedValue &operator=(const RootedValue &) = delete;
    const Value &get() const noexcept {
        return slot_;
    }
    void set(Value value) noexcept {
        slot_ = value;
    }
    void detach() noexcept {
        link_.detach();
    }
    bool belongs_to(const RootRegistry &registry) const noexcept {
        return link_.belongs_to(registry);
    }

  private:
    static void trace_slot(void *, Marker &);
    Value slot_{NullValue{}};
    RootLink link_;
};
// The context must remain stable; its vectors/maps may relocate their own storage.
class NativeRootSet final {
  public:
    NativeRootSet(RootRegistry &registry, void *context, RootTrace trace) noexcept
        : link_(registry, context, trace) {}
    NativeRootSet(const NativeRootSet &) = delete;
    NativeRootSet &operator=(const NativeRootSet &) = delete;
    void detach() noexcept {
        link_.detach();
    }

  private:
    RootLink link_;
};
} // namespace myb
