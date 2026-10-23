#include "diagnostic/failure.hpp"
#include "test_support.hpp"
using namespace myb;
MYB_TEST(failure_first) {
    EmergencyFailure first;
    first.record_first({FailureDomain::Runtime, ReasonCode::Type, {}, {}});
    first.record_first({FailureDomain::Resource, ReasonCode::Allocation, {}, {}});
    REQUIRE(first.is_set());
    REQUIRE(first.header().domain == FailureDomain::Runtime);
    REQUIRE(exit_code(first.header()) == 1);
}
MYB_TEST(failure_categories) {
    EmergencyFailure first;
    try {
        fail({first, SourceSpan{7, 3, 4}}, FailureDomain::Runtime, ReasonCode::Type, "bad type",
             42);
        REQUIRE(false);
    } catch (const MybRuntimeFailure &f) {
        REQUIRE(f.header().span->source == 7);
        REQUIRE(f.header().os_code == 42);
        REQUIRE(std::string_view(f.what()) == "bad type");
    }
    try {
        fail({first, {}}, FailureDomain::Resource, ReasonCode::Allocation, "out of memory");
        REQUIRE(false);
    } catch (const HostFailure &f) {
        REQUIRE(exit_code(f.header()) == 3);
    }
    REQUIRE(first.header().domain == FailureDomain::Runtime);
    REQUIRE(exit_code(first.header()) == 1);
    EmergencyFailure frontend;
    try {
        fail({frontend, {}}, FailureDomain::Structure, ReasonCode::Declaration,
             "duplicate declaration");
        REQUIRE(false);
    } catch (const FrontendFailure &f) {
        REQUIRE(exit_code(f.header()) == 1);
        REQUIRE(!f.header().span);
    }
}
