// Minimal dependency-free test harness, so that "clone, build, run" needs nothing extra.
#pragma once
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace t {

struct Case {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline int& failures() {
    static int count = 0;
    return count;
}

// Constructing one of these (at program start-up) registers a test case.
struct Registrar {
    Registrar(const char* name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

}  // namespace t

// TEST(my_test) { ... }  defines a test and registers it automatically.
#define TEST(name)                                  \
    static void name();                             \
    static t::Registrar registrar_##name(#name, name); \
    static void name()

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("    FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);  \
            ++t::failures();                                                 \
        }                                                                    \
    } while (0)

#define CHECK_EQ(a, b) CHECK((a) == (b))

#define CHECK_NEAR(a, b, eps)                                                   \
    do {                                                                        \
        if (std::fabs(static_cast<double>(a) - static_cast<double>(b)) > (eps)) { \
            std::printf("    FAIL %s:%d  %s (%g) != %s (%g)\n", __FILE__, __LINE__, \
                        #a, static_cast<double>(a), #b, static_cast<double>(b));  \
            ++t::failures();                                                    \
        }                                                                       \
    } while (0)
