#pragma once
#include <span>
namespace myb {
class Utf8Sink {
  public:
    virtual ~Utf8Sink() noexcept = default;
    virtual void append(std::span<const char>) = 0;
};
} // namespace myb
