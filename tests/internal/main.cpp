#include "test_hooks.hpp"
#include "test_support.hpp"
#include <iostream>
#include <string_view>

int main(int argc, char **argv) {
    std::string_view selected;
    if (argc == 3 && std::string_view(argv[1]) == "--case")
        selected = argv[2];
    else if (argc != 1) {
        std::cerr << "usage: myb_internal_tests [--case <name>]\n";
        return 2;
    }
    std::size_t ran = 0;
    std::size_t failed = 0;
    for (const auto &test : myb::test_cases()) {
        if (!selected.empty() && test.name != selected)
            continue;
        ++ran;
        myb::TestHooks::clear_fault();
        myb::TestHooks::force_gc(false);
        try {
            test.run();
            std::cout << "PASS " << test.name << '\n';
        } catch (const std::exception &failure) {
            ++failed;
            std::cerr << "FAIL " << test.name << ": " << failure.what() << '\n';
        } catch (...) {
            ++failed;
            std::cerr << "FAIL " << test.name << ": unknown exception\n";
        }
    }
    myb::TestHooks::clear_fault();
    myb::TestHooks::force_gc(false);
    if (ran == 0) {
        std::cerr << "no test cases matched\n";
        return 2;
    }
    std::cout << ran - failed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
