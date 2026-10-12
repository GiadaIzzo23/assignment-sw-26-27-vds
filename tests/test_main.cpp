#include <cstdio>

#include "check.hpp"

TEST(harness_works) {
    CHECK(1 + 1 == 2);
    CHECK_EQ(2 * 3, 6);
    CHECK_NEAR(0.1 + 0.2, 0.3, 1e-9);
}

int main() {
    for (const auto& test_case : t::registry()) {
        const int failures_before = t::failures();
        test_case.fn();
        const bool ok = t::failures() == failures_before;
        std::printf("[%s] %s\n", ok ? " OK " : "FAIL", test_case.name.c_str());
    }
    std::printf("\n%zu tests, %d failed\n", t::registry().size(), t::failures());
    return t::failures() == 0 ? 0 : 1;
}
