#include "diagnostic/failure.hpp"
#include "source/source.hpp"
#include "test_support.hpp"
using namespace myb;
MYB_TEST(source_positions) {
    EmergencyFailure first;
    SourceManager s(first);
    auto id = s.add("p.myb", "\xEF\xBB\xBF"
                             "\xE4\xB8\xAD\t\xF0\x9F\x98\x80\r\nx\ry\n");
    REQUIRE(s.get(id).bytes.size() == 17);
    REQUIRE(s.locate(id, 7).line == 1);
    REQUIRE(s.locate(id, 7).column == 3);
    REQUIRE(s.locate(id, 13).line == 2);
    REQUIRE(s.locate(id, 15).line == 3);
    REQUIRE(s.locate(id, 17).line == 4);
    REQUIRE(s.slice({id, 3, 11}).size() == 8);
    auto empty = s.add("empty", "");
    REQUIRE(s.locate(empty, 0).column == 1);
    auto sep = s.add("sep", "a\xE2\x80\xA8"
                            "b");
    REQUIRE(s.locate(sep, 4).line == 1);
    try {
        s.add("bad", "ok\n\xFF");
        REQUIRE(false);
    } catch (const FrontendFailure &f) {
        REQUIRE(f.header().span->begin == 3);
    }
}
#include "platform/host_io.hpp"
#include <filesystem>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
MYB_TEST(entry_io) {
    EmergencyFailure first;
    WindowsHostIO io(first);
    auto path = std::filesystem::absolute(std::filesystem::path("build") /
                                          std::filesystem::path(L"\u5165\u53E3 \u4E2D\u6587.myb"));
    auto path_utf8 = wide_to_utf8(path.wstring(), {first, {}});
    const std::string bytes("a\r\n\0\xF0\x9F\x98\x80", 8);
    auto file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(file != INVALID_HANDLE_VALUE);
    DWORD written = 0;
    REQUIRE(WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr));
    REQUIRE(written == bytes.size());
    REQUIRE(CloseHandle(file));
    auto entry = io.read_entry(path_utf8);
    REQUIRE(entry.bytes == bytes);
    REQUIRE(entry.path_utf8 == path_utf8);
    REQUIRE(std::filesystem::remove(path));
    try {
        io.read_entry(path_utf8);
        REQUIRE(false);
    } catch (const HostFailure &h) {
        REQUIRE(h.header().domain == FailureDomain::EntryIO);
        REQUIRE(exit_code(h.header()) == 2);
        REQUIRE(!h.header().span);
        REQUIRE(h.header().os_code.has_value());
    }
    HANDLE reader{}, writer{};
    REQUIRE(CreatePipe(&reader, &writer, nullptr, 0));
    auto old = GetStdHandle(STD_OUTPUT_HANDLE);
    REQUIRE(SetStdHandle(STD_OUTPUT_HANDLE, writer));
    try {
        io.write_stdout(bytes, false);
    } catch (...) {
        SetStdHandle(STD_OUTPUT_HANDLE, old);
        CloseHandle(writer);
        CloseHandle(reader);
        throw;
    }
    REQUIRE(SetStdHandle(STD_OUTPUT_HANDLE, old));
    REQUIRE(CloseHandle(writer));
    char output[32];
    DWORD count = 0;
    REQUIRE(ReadFile(reader, output, 32, &count, nullptr));
    REQUIRE(CloseHandle(reader));
    REQUIRE(std::string(output, count) == bytes);
}
#include "source/utf8.hpp"
MYB_TEST(source_utf8_boundaries) {
    for (auto bad :
         {std::string("\x80", 1), std::string("\xC0\x80", 2), std::string("\xED\xA0\x80", 3),
          std::string("\xF4\x90\x80\x80", 4), std::string("\xE2\x82", 2)}) {
        EmergencyFailure first;
        SourceManager s(first);
        try {
            s.add("bad", std::string("ok\r\n") + bad);
            REQUIRE(false);
        } catch (const FrontendFailure &f) {
            REQUIRE(f.header().span->begin == 4);
            REQUIRE(s.locate(f.header().span->source, 4).line == 2);
            REQUIRE(s.locate(f.header().span->source, 4).column == 1);
        }
    }
    EmergencyFailure first;
    SourceManager s(first);
    auto id = s.add("x", "a\r\nb");
    REQUIRE(s.locate(id, 2).line == 1);
    REQUIRE(s.locate(id, 2).column == 2);
    REQUIRE(s.locate(id, 3).line == 2);
    auto second = s.add("y", "c");
    try {
        s.merge({id, 0, 1}, {second, 0, 1});
        REQUIRE(false);
    } catch (const HostFailure &f) {
        REQUIRE(f.header().domain == FailureDomain::Internal);
    }
    try {
        s.slice({id, 0, 999});
        REQUIRE(false);
    } catch (const HostFailure &f) {
        REQUIRE(f.header().reason == ReasonCode::Invariant);
    }
    REQUIRE(utf8::count_code_points(std::string_view("\0\xF0\x9F\x98\x80", 5)) == 2);
}
