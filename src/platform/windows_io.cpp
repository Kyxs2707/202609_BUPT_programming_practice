#include "platform/host_io.hpp"
#include "source/utf8.hpp"
#define WIN32_LEAN_AND_MEAN
#include <algorithm>
#include <limits>
#include <vector>
#include <windows.h>
namespace myb {
namespace {
[[noreturn]] void io_error(std::string_view path, ErrorSite site, FailureDomain domain,
                           std::string_view message, std::uint32_t code) {
    site.failure.record_first({domain, ReasonCode::IO, site.span, code});
    std::string detail(message);
    detail.append(": ");
    detail.append(path);
    fail(site, domain, ReasonCode::IO, detail, code);
}
struct Handle {
    HANDLE value{INVALID_HANDLE_VALUE};
    ~Handle() noexcept {
        if (value != INVALID_HANDLE_VALUE)
            CloseHandle(value);
    }
    void close(ErrorSite site, FailureDomain domain, std::string_view path) {
        auto h = value;
        value = INVALID_HANDLE_VALUE;
        if (!CloseHandle(h))
            io_error(path, site, domain, "file close failed", GetLastError());
    }
};
void path_check(std::string_view path, ErrorSite site, FailureDomain domain) {
    if (path.find('\0') != path.npos)
        fail(site, domain, ReasonCode::IO, "path contains NUL");
}
std::string read_file(std::string_view path, ErrorSite site, FailureDomain domain) {
    path_check(path, site, domain);
    auto wide = utf8_to_wide(path, site);
    Handle file{CreateFileW(wide.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (file.value == INVALID_HANDLE_VALUE)
        io_error(path, site, domain, "cannot open source file", GetLastError());
    std::string result;
    char buffer[16384];
    for (;;) {
        DWORD n = 0;
        if (!ReadFile(file.value, buffer, sizeof(buffer), &n, nullptr))
            io_error(path, site, domain, "file read failed", GetLastError());
        if (n == 0)
            break;
        if (n > result.max_size() - result.size())
            fail(site, FailureDomain::Resource, ReasonCode::Capacity,
                 "file exceeds string capacity");
        result.append(buffer, n);
    }
    file.close(site, domain, path);
    return result;
}
void write_handle(HANDLE h, std::string_view bytes, ErrorSite site, FailureDomain domain) {
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) {
        auto wide = utf8_to_wide(bytes, site);
        std::size_t at = 0;
        while (at < wide.size()) {
            auto n = std::min<std::size_t>(wide.size() - at, 16384);
            if (at + n < wide.size() && wide[at + n - 1] >= 0xd800 && wide[at + n - 1] <= 0xdbff)
                --n;
            DWORD wrote = 0;
            if (!WriteConsoleW(h, wide.data() + at, static_cast<DWORD>(n), &wrote, nullptr) ||
                wrote == 0)
                fail(site, domain, ReasonCode::IO, "console write failed", GetLastError());
            at += wrote;
        }
    } else {
        std::size_t at = 0;
        while (at < bytes.size()) {
            DWORD wrote = 0;
            auto n = static_cast<DWORD>(std::min<std::size_t>(bytes.size() - at, 65536));
            if (!WriteFile(h, bytes.data() + at, n, &wrote, nullptr) || wrote == 0)
                fail(site, domain, ReasonCode::IO, "standard stream write failed", GetLastError());
            at += wrote;
        }
    }
}
} // namespace
std::wstring utf8_to_wide(std::string_view s, ErrorSite site) {
    if (s.empty())
        return {};
    if (s.size() > INT_MAX)
        fail(site, FailureDomain::Resource, ReasonCode::Capacity,
             "UTF-8 conversion exceeds capacity");
    auto n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(),
                                 static_cast<int>(s.size()), nullptr, 0);
    if (!n)
        fail(site, FailureDomain::Internal, ReasonCode::Encoding,
             "invalid UTF-8 at Windows boundary", GetLastError());
    std::wstring out(n, L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()),
                            out.data(), n) != n)
        fail(site, FailureDomain::Internal, ReasonCode::Encoding, "UTF-8 conversion failed",
             GetLastError());
    return out;
}
std::string wide_to_utf8(std::wstring_view s, ErrorSite site) {
    if (s.empty())
        return {};
    if (s.size() > INT_MAX)
        fail(site, FailureDomain::Resource, ReasonCode::Capacity,
             "UTF-16 conversion exceeds capacity");
    auto n = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s.data(),
                                 static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    if (!n)
        fail(site, FailureDomain::Internal, ReasonCode::Encoding,
             "invalid UTF-16 at Windows boundary", GetLastError());
    std::string out(n, '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()),
                            out.data(), n, nullptr, nullptr) != n)
        fail(site, FailureDomain::Internal, ReasonCode::Encoding, "UTF-16 conversion failed",
             GetLastError());
    return out;
}
EntryBytes WindowsHostIO::read_entry(std::string_view path) {
    const ErrorSite site{failure_, {}};
    path_check(path, site, FailureDomain::EntryIO);
    const auto wide = utf8_to_wide(path, site);
    const auto capacity = GetFullPathNameW(wide.c_str(), 0, nullptr, nullptr);
    if (!capacity)
        io_error(path, site, FailureDomain::EntryIO, "cannot resolve source path", GetLastError());
    std::wstring absolute(capacity, L'\0');
    const auto used = GetFullPathNameW(wide.c_str(), capacity, absolute.data(), nullptr);
    if (!used || used >= capacity)
        io_error(path, site, FailureDomain::EntryIO, "cannot resolve source path", GetLastError());
    absolute.resize(used);
    auto absolute_utf8 = wide_to_utf8(absolute, site);
    auto bytes = read_file(absolute_utf8, site, FailureDomain::EntryIO);
    return {std::move(absolute_utf8), std::move(bytes)};
}
void WindowsHostIO::write_stdout(std::string_view bytes, bool lf) {
    write_handle(GetStdHandle(STD_OUTPUT_HANDLE), bytes, {failure_, {}}, FailureDomain::Runtime);
    if (lf)
        write_handle(GetStdHandle(STD_OUTPUT_HANDLE), "\n", {failure_, {}}, FailureDomain::Runtime);
}
void WindowsHostIO::write_stderr(std::string_view bytes) {
    write_handle(GetStdHandle(STD_ERROR_HANDLE), bytes, {failure_, {}}, FailureDomain::Internal);
}
void WindowsHostIO::read_text(std::string_view, Utf8Sink &) {
    fail({failure_, {}}, FailureDomain::Internal, ReasonCode::Invariant,
         "read_text is unavailable in the M1 subset");
}
void WindowsHostIO::write_text(std::string_view, std::string_view) {
    fail({failure_, {}}, FailureDomain::Internal, ReasonCode::Invariant,
         "write_text is unavailable in the M1 subset");
}
LineStatus WindowsHostIO::read_line(Utf8Sink &) {
    fail({failure_, {}}, FailureDomain::Internal, ReasonCode::Invariant,
         "read_line is unavailable in the M1 subset");
}
} // namespace myb
