// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#ifndef ZGUI_TEST_HARNESS_HH
#define ZGUI_TEST_HARNESS_HH

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace zgui_test {

    struct TestCase {
        const char* name;
        void (*run)();
    };

    inline std::vector<TestCase>& registry() {
        static std::vector<TestCase> tests;
        return tests;
    }

    inline std::size_t& failure_count() {
        static std::size_t failures = 0;
        return failures;
    }

    struct Registrar {
        Registrar(const char* name, void (*run)()) {
            registry().push_back({name, run});
        }
    };

    inline void check(bool result, const char* expression, const char* file, int line) {
        if (result)
            return;
        ++failure_count();
        std::cerr << file << ':' << line << ": CHECK failed: " << expression << '\n';
    }

    inline int run_all_tests() {
        std::size_t passed = 0;
        for (const TestCase& test : registry()) {
            const std::size_t failures_before = failure_count();
            test.run();
            if (failure_count() == failures_before) {
                ++passed;
                std::cout << "[PASS] " << test.name << '\n';
            } else {
                std::cout << "[FAIL] " << test.name << '\n';
            }
        }
        std::cout << passed << '/' << registry().size() << " tests passed; " << failure_count()
                  << " failed checks\n";
        return failure_count() == 0 ? 0 : 1;
    }

} // namespace zgui_test

#define ZGUI_TEST_JOIN_IMPL(left, right) left##right
#define ZGUI_TEST_JOIN(left, right) ZGUI_TEST_JOIN_IMPL(left, right)
#define TEST(name)                                                                                 \
    static void name();                                                                            \
    static const ::zgui_test::Registrar ZGUI_TEST_JOIN(registrar_, name)(#name, &name);            \
    static void name()
#define CHECK(expression)                                                                          \
    ::zgui_test::check(static_cast<bool>(expression), #expression, __FILE__, __LINE__)
#define CHECK_EQ(left, right) CHECK((left) == (right))
#define CHECK_NEAR(left, right, tolerance)                                                         \
    CHECK(std::fabs(static_cast<double>(left) - static_cast<double>(right)) <=                     \
          static_cast<double>(tolerance))

#endif // ZGUI_TEST_HARNESS_HH
