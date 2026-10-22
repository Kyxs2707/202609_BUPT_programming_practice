#pragma once
#include "diagnostic/failure.hpp"
#include "source/span.hpp"
#include <string>
#include <string_view>
#include <vector>
namespace myb {
struct SourceFile {
    std::string path_utf8, bytes;
    std::vector<std::size_t> line_starts;
    std::size_t bom_size{};
};
class SourceManager {
  public:
    explicit SourceManager(EmergencyFailure &first) : failure_(first) {}
    SourceId add(std::string path_utf8, std::string bytes);
    const SourceFile &get(SourceId) const;
    SourcePosition locate(SourceId, std::uint64_t) const;
    std::string_view slice(SourceSpan) const;
    EmergencyFailure &failure() const noexcept {
        return failure_;
    }
    SourceSpan merge(SourceSpan, SourceSpan) const;

  private:
    EmergencyFailure &failure_;
    std::vector<SourceFile> files_;
};
} // namespace myb
