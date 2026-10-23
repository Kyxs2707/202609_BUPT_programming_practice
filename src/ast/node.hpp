#pragma once
#include "source/span.hpp"
#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>
namespace myb {
class AstVisitor;
class Node {
  public:
    Node(SourceSpan s, std::uint16_t h) : span_(s), height_(h) {}
    virtual ~Node() noexcept = default;
    SourceSpan span() const noexcept {
        return span_;
    }
    std::uint16_t height() const noexcept {
        return height_;
    }
    virtual void accept(AstVisitor &) const = 0;

  private:
    SourceSpan span_;
    std::uint16_t height_;
};
class TopItem : public Node {
  public:
    using Node::Node;
};
class Stmt : public TopItem {
  public:
    using TopItem::TopItem;
};
class Expr : public Node {
  public:
    using Node::Node;
};
template <class T> class ReadOnlyNodes {
  public:
    explicit ReadOnlyNodes(std::vector<std::unique_ptr<T>> values) : values_(std::move(values)) {}
    std::size_t size() const noexcept {
        return values_.size();
    }
    bool empty() const noexcept {
        return values_.empty();
    }
    const T &operator[](std::size_t i) const {
        return *values_[i];
    }
    class Iterator {
        using Base = typename std::vector<std::unique_ptr<T>>::const_iterator;
        Base i_;

      public:
        explicit Iterator(Base i) : i_(i) {}
        const T &operator*() const {
            return **i_;
        }
        Iterator &operator++() {
            ++i_;
            return *this;
        }
        bool operator!=(const Iterator &o) const {
            return i_ != o.i_;
        }
    };
    Iterator begin() const {
        return Iterator(values_.begin());
    }
    Iterator end() const {
        return Iterator(values_.end());
    }

  private:
    std::vector<std::unique_ptr<T>> values_;
};
} // namespace myb
