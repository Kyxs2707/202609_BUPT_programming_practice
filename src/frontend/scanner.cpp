#include "frontend/scanner.hpp"
#include <cstring>
#include <limits>
int yylex_init_extra(myb::ScannerContext *, yyscan_t *);
int yylex_destroy(yyscan_t);
yy_buffer_state *yy_scan_buffer(char *, std::size_t, yyscan_t);
myb::ScannerContext *yyget_extra(yyscan_t);
namespace myb {
Scanner::Scanner(SourceManager &sources, SourceId id) : context_(id, sources.failure()) {
    const auto &bytes = sources.get(id).bytes;
    // Flex 2.6.4 yy_scan_buffer takes size_t, but stores size - 2 in int.
    if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        bytes.size() > input_.max_size() - 2)
        fail({context_.failure, {}}, FailureDomain::Resource, ReasonCode::Capacity,
             "scanner input exceeds generated backend capacity");
    input_.reserve(bytes.size() + 2);
    input_.insert(input_.end(), bytes.begin(), bytes.end());
    input_.push_back(0);
    input_.push_back(0);
    try {
        if (yylex_init_extra(&context_, &handle_) != 0)
            fail({context_.failure, {}}, FailureDomain::Resource, ReasonCode::Allocation,
                 "scanner initialization failed");
        buffer_ = yy_scan_buffer(input_.data(), input_.size(), handle_);
        if (!buffer_)
            fail({context_.failure, {}}, FailureDomain::Internal, ReasonCode::Invariant,
                 "scanner rejected input buffer");
    } catch (...) {
        if (handle_) {
            yylex_destroy(handle_);
            handle_ = nullptr;
        }
        context_.allocations.clear();
        throw;
    }
}
Scanner::~Scanner() noexcept {
    if (handle_)
        yylex_destroy(handle_);
    context_.allocations.clear();
}
Parser::symbol_type Scanner::next() {
    return raw_lex(handle_);
}
void ScannerContext::escape(std::string_view text) {
    if (text[1] != 'u') {
        char c = text[1];
        switch (c) {
        case 'n':
            c = '\n';
            break;
        case 'r':
            c = '\r';
            break;
        case 't':
            c = '\t';
            break;
        default:
            break;
        }
        decoded_string.push_back(c);
        return;
    }
    std::uint32_t cp = 0;
    for (std::size_t i = 3; i + 1 < text.size(); ++i) {
        auto c = text[i];
        cp = (cp << 4) | static_cast<unsigned>(c <= '9'   ? c - '0'
                                               : c <= 'F' ? c - 'A' + 10
                                                          : c - 'a' + 10);
    }
    if (cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
        lexical_error("Unicode escape is not a scalar value");
    if (cp < 0x80)
        decoded_string.push_back(static_cast<char>(cp));
    else if (cp < 0x800) {
        decoded_string.push_back(static_cast<char>(0xc0 | (cp >> 6)));
        decoded_string.push_back(static_cast<char>(0x80 | (cp & 63)));
    } else if (cp < 0x10000) {
        decoded_string.push_back(static_cast<char>(0xe0 | (cp >> 12)));
        decoded_string.push_back(static_cast<char>(0x80 | ((cp >> 6) & 63)));
        decoded_string.push_back(static_cast<char>(0x80 | (cp & 63)));
    } else {
        decoded_string.push_back(static_cast<char>(0xf0 | (cp >> 18)));
        decoded_string.push_back(static_cast<char>(0x80 | ((cp >> 12) & 63)));
        decoded_string.push_back(static_cast<char>(0x80 | ((cp >> 6) & 63)));
        decoded_string.push_back(static_cast<char>(0x80 | (cp & 63)));
    }
}
[[noreturn]] void flex_fatal(const char *text, yyscan_t scanner) {
    auto *c = yyget_extra(scanner);
    bool allocation = std::strstr(text, "memory") || std::strstr(text, "alloc");
    bool capacity = std::strstr(text, "overflow") || std::strstr(text, "capacity");
    fail({c->failure, {}},
         allocation || capacity ? FailureDomain::Resource : FailureDomain::Internal,
         capacity     ? ReasonCode::Capacity
         : allocation ? ReasonCode::Allocation
                      : ReasonCode::Invariant,
         text);
}
} // namespace myb
void *yyalloc(std::size_t n, yyscan_t scanner) {
    return yyget_extra(scanner)->allocations.allocate(n);
}
void *yyrealloc(void *p, std::size_t n, yyscan_t scanner) {
    return yyget_extra(scanner)->allocations.reallocate(p, n);
}
void yyfree(void *p, yyscan_t scanner) {
    yyget_extra(scanner)->allocations.release(p);
}
