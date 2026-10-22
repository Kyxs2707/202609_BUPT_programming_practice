#pragma once
#include "frontend/driver.hpp"
#include "frontend/flex_allocations.hpp"
#include "parser.hpp"
#include <vector>
typedef void *yyscan_t;
struct yy_buffer_state;
namespace myb {
struct ScannerContext {
    SourceId source;
    std::uint64_t consumed{};
    SourceSpan matched{};
    std::uint64_t string_begin{};
    std::string decoded_string;
    EmergencyFailure &failure;
    FlexAllocationLedger allocations;
    ScannerContext(SourceId s, EmergencyFailure &f) : source(s), failure(f), allocations(f) {}
    void match(std::size_t n) {
        matched = {source, consumed, consumed + n};
        consumed += n;
    }
    SourceSpan eof() const noexcept {
        return {source, consumed, consumed};
    }
    [[noreturn]] void lexical_error(std::string_view message) {
        fail({failure, matched}, FailureDomain::Frontend, ReasonCode::Syntax, message);
    }
    void escape(std::string_view);
};
class Scanner {
  public:
    Scanner(SourceManager &, SourceId);
    ~Scanner() noexcept;
    Scanner(const Scanner &) = delete;
    Scanner &operator=(const Scanner &) = delete;
    Parser::symbol_type next();

  private:
    ScannerContext context_;
    yyscan_t handle_{};
    yy_buffer_state *buffer_{};
    std::vector<char> input_;
};
[[noreturn]] void flex_fatal(const char *, yyscan_t);
} // namespace myb
myb::Parser::symbol_type raw_lex(yyscan_t);
