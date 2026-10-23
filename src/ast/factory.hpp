#pragma once
#include "ast/nodes.hpp"
#include "diagnostic/failure.hpp"
#include <algorithm>
#include <utility>
namespace myb {
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
