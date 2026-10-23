#include "frontend/driver.hpp"
#include "frontend/scanner.hpp"
namespace myb {
Driver::Driver(SourceManager &sources, SourceId source)
    : sources_(sources), source_(source), factory_(sources.failure()) {}
Driver::~Driver() noexcept = default;
Scanner &Driver::scanner() {
    if (!scanner_)
        scanner_ = std::make_unique<Scanner>(sources_, source_);
    return *scanner_;
}
std::unique_ptr<AstProgram> Driver::parse() {
    program_.reset();
    scanner_.reset();
    try {
        Parser parser(*this);
        if (parser.parse() != 0)
            fail({failure(), {}}, FailureDomain::Internal, ReasonCode::Invariant,
                 "parser failed without a diagnostic");
        scanner_.reset();
        return std::move(program_);
    } catch (...) {
        // 解析器栈已在展开时释放语义值，此处清理驱动独占的阶段产物。
        scanner_.reset();
        program_.reset();
        throw;
    }
}
Parser::symbol_type yylex(Driver &driver) {
    return driver.scanner().next();
}
} // namespace myb
