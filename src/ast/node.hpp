#pragma once
#include "source/span.hpp"
#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>
namespace myb {
class AstVisitor;
/**
 * @brief 保存源码范围和子树高度，并提供只读节点分派。
 *
 * 节点通过 AstFactory 构造；父节点独占子节点，名称和字面量由节点拥有。
 * 派生节点的字符串视图、序列视图、子节点引用和指针均借用所属树，
 * 使用方负责使树存活至访问结束。节点中的宿主数据不属于受管堆。
 * span() 返回值副本；height() 返回从本节点到最深叶节点的节点数。
 */
class Node {
  public:
    /**
     * @brief 初始化构造后保持不变的节点位置和高度。
     * @param[in] s 节点覆盖的源码范围。
     * @param[in] h 包含本节点的子树高度。
     * @pre h 为工厂核验后的高度，范围为 1～256。
     */
    Node(SourceSpan s, std::uint16_t h) : span_(s), height_(h) {}
    virtual ~Node() noexcept = default;
    SourceSpan span() const noexcept {
        return span_;
    }
    std::uint16_t height() const noexcept {
        return height_;
    }
    /**
     * @brief 将本节点分派给对应的 visit 重载。
     * 遍历子节点的顺序和阶段状态由访问者管理；此入口只分派本节点。
     */
    virtual void accept(AstVisitor &) const = 0;

  private:
    SourceSpan span_;
    std::uint16_t height_;
};
/** @brief 文件顶层项目的基类，供声明和普通语句共用。 */
class TopItem : public Node {
  public:
    using Node::Node;
};
/** @brief 可出现在文件顶层或语句块中的语句基类。 */
class Stmt : public TopItem {
  public:
    using TopItem::TopItem;
};
/** @brief 表达式节点基类；求值与检查结果由各阶段单独保存。 */
class Expr : public Node {
  public:
    using Node::Node;
};
/**
 * @brief 独占节点序列，并仅向调用方提供 const 节点访问。
 * @tparam T 序列保存的节点基类。
 *
 * 元素按传入顺序保存，构造时接收全部 unique_ptr 的所有权。
 * 节点引用在对应节点销毁后失效；迭代器借用所属序列，调用方须在移动前结束迭代。
 * @pre values 中每个指针均非空；下标访问要求 i 小于 size()。
 */
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
    /** @brief 通过 const T& 访问所属序列中的节点。 */
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
