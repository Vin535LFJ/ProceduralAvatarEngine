#pragma once

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace pae {
namespace test {

struct Registry {
    std::vector<std::pair<std::string, std::function<void()>>> cases;
    int failures = 0;

    static Registry& instance() {
        static Registry r;
        return r;
    }

    void add(const char* name, std::function<void()> fn) {
        cases.emplace_back(name, std::move(fn));
    }

    void expect(bool condition, const char* expression, const char* file, int line) {
        if (condition) return;
        ++failures;
        std::fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expression);
    }

    template <typename A, typename B>
    void expectNear(A a, B b, double tolerance, const char* exprA, const char* exprB,
                    const char* file, int line) {
        const double diff = std::fabs(static_cast<double>(a) - static_cast<double>(b));
        if (diff <= tolerance && std::isfinite(diff)) return;
        ++failures;
        std::fprintf(stderr, "FAIL %s:%d: |%s - %s| = %.9g > %.9g\n",
                     file, line, exprA, exprB, diff, tolerance);
    }

    int run() {
        for (auto& item : cases) {
            const int before = failures;
            std::fprintf(stderr, "[ RUN ] %s\n", item.first.c_str());
            item.second();
            if (failures == before) {
                std::fprintf(stderr, "[  OK ] %s\n", item.first.c_str());
            } else {
                std::fprintf(stderr, "[FAIL ] %s\n", item.first.c_str());
            }
            std::fflush(stderr);
        }
        std::fprintf(stderr, "%zu cases, %d failures\n", cases.size(), failures);
        std::fflush(stderr);
        return failures == 0 ? 0 : 1;
    }
};

struct Registrar {
    Registrar(const char* name, std::function<void()> fn) {
        Registry::instance().add(name, std::move(fn));
    }
};

inline int runAllTests() { return Registry::instance().run(); }

} // namespace test
} // namespace pae

#define PAE_TEST_CONCAT_INNER(a, b) a##b
#define PAE_TEST_CONCAT(a, b) PAE_TEST_CONCAT_INNER(a, b)
#define PAE_TEST_CASE(name) \
    static void name(); \
    static ::pae::test::Registrar PAE_TEST_CONCAT(reg_, name)(#name, name); \
    static void name()

#define EXPECT_TRUE(expr) \
    ::pae::test::Registry::instance().expect(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))
#define EXPECT_NEAR(a, b, tol) \
    ::pae::test::Registry::instance().expectNear((a), (b), (tol), #a, #b, __FILE__, __LINE__)
