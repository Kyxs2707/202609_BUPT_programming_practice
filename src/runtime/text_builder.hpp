#pragma once
#include "platform/utf8_sink.hpp"
#include <string>
#include <string_view>

namespace myb {
class Heap;
class TextBuilder final : public Utf8Sink {
  public:
    explicit TextBuilder(Heap &);
    ~TextBuilder() noexcept override;
    TextBuilder(const TextBuilder &) = delete;
    TextBuilder &operator=(const TextBuilder &) = delete;
    void append(std::span<const char>) override;
    void reserve(std::size_t);
    std::string_view bytes() const noexcept {
        return buffer_;
    }

  private:
    friend class Heap;
    Heap &heap_;
    std::string buffer_;
    std::size_t accounted_capacity_{};
    bool consumed_{};
};
} // namespace myb
