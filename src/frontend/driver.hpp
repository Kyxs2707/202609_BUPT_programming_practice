#pragma once
#include "ast/factory.hpp"
#include "source/source.hpp"
#include <memory>
#include <string>
namespace myb {
struct IntLexeme {
    std::string digits;
};
struct FloatLexeme {
    std::string text;
};
class Scanner;
class Driver {
  public:
    Driver(SourceManager &, SourceId);
    ~Driver() noexcept;
    std::unique_ptr<AstProgram> parse();
    Scanner &scanner();
    AstFactory &factory() noexcept {
        return factory_;
    }
    SourceId source_id() const noexcept {
        return source_;
    }
    EmergencyFailure &failure() const noexcept {
        return sources_.failure();
    }
    void accept(std::unique_ptr<AstProgram> program) {
        program_ = std::move(program);
    }

  private:
    SourceManager &sources_;
    SourceId source_;
    AstFactory factory_;
    std::unique_ptr<Scanner> scanner_;
    std::unique_ptr<AstProgram> program_;
};
} // namespace myb
