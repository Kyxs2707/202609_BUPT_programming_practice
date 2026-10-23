#include "cli/options.hpp"
#include "fixtures.hpp"
#include "test_hooks.hpp"
#include "test_support.hpp"
#include <string>

using namespace myb;

MYB_TEST(smoke_pipeline) {
    VmFixture f;
    auto ok = f.run("print(1 + 2);");
    REQUIRE(ok.exit_code == 0);
    REQUIRE(ok.stdout_bytes == "3\n");
    REQUIRE(ok.stderr_bytes.empty());
    auto division = f.run("print(1 / 0);");
    REQUIRE(division.exit_code == 1);
    REQUIRE(division.failure->reason == ReasonCode::Arithmetic);
    REQUIRE(division.stdout_bytes.empty());
    auto syntax = f.run("print(1 + );");
    REQUIRE(syntax.exit_code == 1);
    REQUIRE(syntax.stdout_bytes.empty());
    auto late = f.run("print(1); print(2 + );");
    REQUIRE(late.exit_code == 1);
    REQUIRE(late.stdout_bytes.empty());
    auto arity = f.run("print(write(\"X\"), 1);");
    REQUIRE(arity.exit_code == 1);
    REQUIRE(arity.failure->reason == ReasonCode::Type);
    REQUIRE(arity.stdout_bytes.empty());
    auto name = f.run("unknown(write(\"X\"));");
    REQUIRE(name.exit_code == 1);
    REQUIRE(name.stdout_bytes.empty());
    auto check = f.check("print(write(\"X\"), 1); read_line(); print(1/0);");
    REQUIRE(check.exit_code == 0);
    REQUIRE(check.stdout_bytes.empty() && check.stderr_bytes.empty());
    REQUIRE(check.stdin_reads == 0 && check.native_calls == 0);
    auto numeric = f.run("print(-9223372036854775808); print(1.0 + 2); print(-0.0);");
    REQUIRE(numeric.exit_code == 0);
    REQUIRE(numeric.stdout_bytes == "-9223372036854775808\n3.0\n-0.0\n");
    auto invalid_min = f.check("print(-(9223372036854775808));");
    REQUIRE(invalid_min.exit_code == 1);
    TestHooks::force_gc(true);
    auto text = f.run("print(\"北邮😀\"); print(\"a\\u{0}b\");");
    TestHooks::force_gc(false);
    REQUIRE(text.exit_code == 0);
    REQUIRE(text.stdout_bytes == std::string("北邮😀\na\0b\n", 15));
}

MYB_TEST(cli_options) {
    const std::string_view a[] = {"--check", "--", "-sample"};
    auto parsed = parse_options(a);
    REQUIRE(parsed.mode == RunMode::Check && parsed.file == "-sample");
    for (const auto &args : {std::vector<std::string_view>{},
                             {"--bad"},
                             {"a", "b"},
                             {"a", "--check"},
                             {"--check", "--check", "a"},
                             {"--help", "a"}}) {
        bool failed = false;
        try {
            (void)parse_options(args);
        } catch (const Failure &ex) {
            failed = exit_code(ex.header()) == 2;
        }
        REQUIRE(failed);
    }
}

MYB_TEST(smoke_resource) {
    VmFixture f;
    TestHooks::fail_next(FaultPoint::VmValues);
    auto result = f.run("print(1 + 2);");
    TestHooks::clear_fault();
    REQUIRE(result.exit_code == 3);
    REQUIRE(result.failure && result.failure->domain == FailureDomain::Resource);
    REQUIRE(result.stdout_bytes.empty());
}
