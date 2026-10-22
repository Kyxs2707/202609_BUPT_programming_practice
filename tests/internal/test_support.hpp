#pragma once
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace myb {
struct TestCase {
    std::string_view name;
    void (*run)();
};
inline std::vector<TestCase> &test_cases() {
    static std::vector<TestCase> cases;
    return cases;
}
struct TestRegistration {
    TestRegistration(std::string_view name, void (*run)()) {
        test_cases().push_back({name, run});
    }
};
inline void require(bool condition, const char *expression, const char *file, int line) {
    if (!condition) {
        std::ostringstream message;
        message << file << ':' << line << ": REQUIRE(" << expression << ") failed";
        throw std::runtime_error(message.str());
    }
}
} // namespace myb

#define MYB_TEST(name)                                                                             \
    static void myb_test_##name();                                                                 \
    static ::myb::TestRegistration myb_registration_##name(#name, &myb_test_##name);               \
    static void myb_test_##name()
#define REQUIRE(...)                                                                               \
    ::myb::require(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __FILE__, __LINE__)
