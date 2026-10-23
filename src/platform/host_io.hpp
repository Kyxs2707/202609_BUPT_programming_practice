#pragma once
#include "diagnostic/failure.hpp"
#include "platform/utf8_sink.hpp"
#include <string>
#include <string_view>
namespace myb {
struct EntryBytes {
    std::string path_utf8, bytes;
};
enum class LineStatus { Line, End };
class HostIO {
  public:
    virtual ~HostIO() noexcept = default;
    virtual EntryBytes read_entry(std::string_view) = 0;
    virtual LineStatus read_line(Utf8Sink &) = 0;
    virtual void read_text(std::string_view, Utf8Sink &) = 0;
    virtual void write_text(std::string_view, std::string_view) = 0;
    virtual void write_stdout(std::string_view, bool append_lf) = 0;
    virtual void write_stderr(std::string_view) = 0;
};
std::wstring utf8_to_wide(std::string_view, ErrorSite);
std::string wide_to_utf8(std::wstring_view, ErrorSite);
class WindowsHostIO final : public HostIO {
  public:
    explicit WindowsHostIO(EmergencyFailure &failure) : failure_(failure) {}
    EntryBytes read_entry(std::string_view) override;
    LineStatus read_line(Utf8Sink &) override;
    void read_text(std::string_view, Utf8Sink &) override;
    void write_text(std::string_view, std::string_view) override;
    void write_stdout(std::string_view, bool append_lf) override;
    void write_stderr(std::string_view) override;

  private:
    EmergencyFailure &failure_;
};
} // namespace myb
