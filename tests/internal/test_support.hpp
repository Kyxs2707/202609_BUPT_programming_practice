#pragma once
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace myb {
/** @brief 一个静态名称与无参数测试入口；名称存储须覆盖整个测试进程。 */
struct TestCase {
    std::string_view name;
    void (*run)();
};
/** @brief 借用进程内唯一的测试登记表，由静态登记对象填充。 */
inline std::vector<TestCase> &test_cases() {
    static std::vector<TestCase> cases;
    return cases;
}
/** @brief 在静态初始化期间将测试入口加入登记表。 */
struct TestRegistration {
    TestRegistration(std::string_view name, void (*run)()) {
        test_cases().push_back({name, run});
    }
};
/** @brief 检查 Release 中仍生效的断言，失败时抛出包含文件、行号和表达式的错误。 */
inline void require(bool condition, const char *expression, const char *file, int line) {
    if (!condition) {
        std::ostringstream message;
        message << file << ':' << line << ": REQUIRE(" << expression << ") failed";
        throw std::runtime_error(message.str());
    }
}
} // namespace myb

/** @brief 声明并登记一个内部测试，name 同时生成测试函数名和筛选名称。 */
#define MYB_TEST(name)                                                                             \
    static void myb_test_##name();                                                                 \
    static ::myb::TestRegistration myb_registration_##name(#name, &myb_test_##name);               \
    static void myb_test_##name()
/** @brief 求值断言并保留源码表达式及位置，失败通过 require 报告。 */
#define REQUIRE(...)                                                                               \
    ::myb::require(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __FILE__, __LINE__)
