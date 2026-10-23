#pragma once
#include "ast/nodes.hpp"
#include "diagnostic/failure.hpp"
#include <algorithm>
#include <utility>
namespace myb {
/**
 * @brief 在连接子树前核验高度，统一构造 AST 节点。
 *
 * 借用的 EmergencyFailure 须比工厂存活更久。高度只累计父子路径，
 * 叶节点和空程序的高度均为 1；同一序列中兄弟节点取最大高度。
 */
class AstFactory {
    EmergencyFailure &failure_;
    template <class T> static std::uint16_t child_height(const T &) {
        return 0;
    }
    template <class T> static std::uint16_t child_height(const std::unique_ptr<T> &p) {
        return p ? p->height() : 0;
    }
    template <class T> static std::uint16_t child_height(const std::vector<T> &v) {
        std::uint16_t h = 0;
        for (auto &x : v)
            h = std::max(h, child_height(x));
        return h;
    }
    static std::uint16_t child_height(const FieldInit &f) {
        return f.value().height();
    }

  public:
    explicit AstFactory(EmergencyFailure &f) : failure_(f) {}
    /**
     * @brief 构造高度最多为 256 的节点并交付独占所有权。
     * @tparam T 待构造的具体节点类型。
     * @tparam A 节点语义字段的参数类型。
     * @param[in] span 节点覆盖的源码范围。
     * @param[in] args 转发给节点构造函数的字段，子树所有权随构造转移。
     * @return 独占新节点及其子树的指针。
     * @pre 必选子节点非空，字段满足对应节点的构造约束。
     * @throws HostFailure 新父节点的高度将超过 256，故障为 Resource/Capacity。
     * @throws std::bad_alloc 节点分配失败。
     */
    template <class T, class... A> std::unique_ptr<T> make(SourceSpan span, A &&...args) {
        std::uint16_t h = 0;
        ((h = std::max(h, child_height(args))), ...);
        if (h >= 256)
            fail({failure_, span}, FailureDomain::Resource, ReasonCode::Capacity,
                 "AST height exceeds 256");
        return std::make_unique<T>(span, static_cast<std::uint16_t>(h + 1),
                                   std::forward<A>(args)...);
    }
};
} // namespace myb
