#pragma once
#include "source/span.hpp"
#include <exception>
#include <optional>
#include <string>
#include <string_view>
namespace myb {
enum class FailureDomain { Usage, EntryIO, Frontend, Structure, Runtime, Resource, Internal };
enum class ReasonCode {
    Encoding,
    Syntax,
    Declaration,
    Name,
    Uninitialized,
    Type,
    Range,
    Arithmetic,
    IterationMutation,
    IO,
    Assertion,
    Capacity,
    Allocation,
    Invariant
};
struct FailureHeader {
    FailureDomain domain;
    ReasonCode reason;
    std::optional<SourceSpan> span{};
    std::optional<std::uint32_t> os_code{};
};
class EmergencyFailure {
  public:
    void record_first(FailureHeader h) noexcept {
        if (!set_) {
            header_ = h;
            set_ = true;
        }
    }
    bool is_set() const noexcept {
        return set_;
    }
    const FailureHeader &header() const noexcept {
        return header_;
    }

  private:
    FailureHeader header_{FailureDomain::Internal, ReasonCode::Invariant, {}, {}};
    bool set_{};
};
struct ErrorSite {
    EmergencyFailure &failure;
    std::optional<SourceSpan> span{};
};
class Failure : public std::exception {
  public:
    Failure(FailureHeader h, std::string_view message) : header_(h), message_(message) {}
    const FailureHeader &header() const noexcept {
        return header_;
    }
    const char *what() const noexcept override {
        return message_.c_str();
    }

  private:
    FailureHeader header_;
    std::string message_;
};
class FrontendFailure final : public Failure {
  public:
    using Failure::Failure;
};
class MybRuntimeFailure final : public Failure {
  public:
    using Failure::Failure;
};
class HostFailure final : public Failure {
  public:
    using Failure::Failure;
};
[[noreturn]] void fail(ErrorSite, FailureDomain, ReasonCode, std::string_view,
                       std::optional<std::uint32_t> os_code = {});
int exit_code(const FailureHeader &) noexcept;
std::string_view domain_name(FailureDomain) noexcept;
std::string_view reason_name(ReasonCode) noexcept;
} // namespace myb
