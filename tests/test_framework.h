/**
 * @file test_framework.h
 * @brief Shared lightweight test framework for QuatEngine tests.
 *
 * Provides unified assertion macros and test runner utilities.
 * No external dependencies -- just compile and run.
 *
 * Usage:
 *   #include "test_framework.h"
 *
 *   void test_something() {
 *       ASSERT_TRUE(1 + 1 == 2);
 *       ASSERT_FLOAT_EQ(3.14f, 3.14f, 1e-5f);
 *   }
 *
 *   int main() {
 *       RUN_TEST(test_something);
 *       return TEST_REPORT();
 *   }
 */

#ifndef QE_TEST_FRAMEWORK_H
#define QE_TEST_FRAMEWORK_H

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

// ── Global Counters ─────────────────────────────────────────────────────────

static int g_tests_run    = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

// ── Assertion Macros ────────────────────────────────────────────────────────

#define ASSERT_TRUE(expr)                                                    \
    do {                                                                     \
        ++g_tests_run;                                                       \
        if (!(expr)) {                                                       \
            std::cerr << "  FAIL: " << #expr << " (" << __FILE__ << ":"      \
                      << __LINE__ << ")" << std::endl;                       \
            ++g_tests_failed;                                                \
        } else {                                                             \
            ++g_tests_passed;                                                \
        }                                                                    \
    } while (0)

#define ASSERT_FLOAT_EQ(a, b, eps)                                           \
    do {                                                                     \
        ++g_tests_run;                                                       \
        if (std::abs((a) - (b)) > (eps)) {                                   \
            std::cerr << "  FAIL: " << #a << " == " << #b                    \
                      << " (got " << (a) << " vs " << (b)                    \
                      << ", eps=" << (eps) << ") at " << __FILE__            \
                      << ":" << __LINE__ << std::endl;                       \
            ++g_tests_failed;                                                \
        } else {                                                             \
            ++g_tests_passed;                                                \
        }                                                                    \
    } while (0)

/** Alias matching the shorter form used in many test files. */
#define ASSERT_NEAR(a, b, eps) ASSERT_TRUE(std::abs((a)-(b)) < (eps))

#define ASSERT_VEC3_EQ(v, ex, ey, ez, eps)                                   \
    do {                                                                     \
        ASSERT_FLOAT_EQ((v).x, (ex), (eps));                                 \
        ASSERT_FLOAT_EQ((v).y, (ey), (eps));                                 \
        ASSERT_FLOAT_EQ((v).z, (ez), (eps));                                 \
    } while (0)

/** Expect any exception from expr. */
#define ASSERT_THROWS(expr)                                                  \
    do {                                                                     \
        ++g_tests_run;                                                       \
        bool threw_ = false;                                                 \
        try { (void)(expr); } catch (...) { threw_ = true; }                 \
        if (!threw_) {                                                       \
            std::cerr << "  FAIL: expected throw from " << #expr             \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            ++g_tests_failed;                                                \
        } else {                                                             \
            ++g_tests_passed;                                                \
        }                                                                    \
    } while (0)

/** Expect a specific exception type from expr. */
#define ASSERT_THROWS_AS(expr, exception_type)                               \
    do {                                                                     \
        ++g_tests_run;                                                       \
        bool caught_ = false;                                                \
        try { (void)(expr); } catch (const exception_type&) { caught_ = true; } \
        if (!caught_) {                                                      \
            std::cerr << "  FAIL: expected " #exception_type " from "        \
                      << #expr << " at " << __FILE__ << ":"                  \
                      << __LINE__ << std::endl;                              \
            ++g_tests_failed;                                                \
        } else {                                                             \
            ++g_tests_passed;                                                \
        }                                                                    \
    } while (0)

// ── Test Runner ─────────────────────────────────────────────────────────────

#define RUN_TEST(test_fn)                                                    \
    do {                                                                     \
        std::cout << "  " << #test_fn << "... ";                             \
        int before_fail_ = g_tests_failed;                                   \
        test_fn();                                                           \
        std::cout << (g_tests_failed == before_fail_ ? "OK" : "FAILED")      \
                  << std::endl;                                              \
    } while (0)

// ── Results ─────────────────────────────────────────────────────────────────

/** Print summary and return appropriate exit code for main(). */
#define TEST_REPORT()                                                        \
    [&]() -> int {                                                           \
        std::cout << "\n=== Results ===" << std::endl;                       \
        std::cout << "  Total: " << g_tests_run << std::endl;               \
        std::cout << "  Passed: " << g_tests_passed << std::endl;           \
        std::cout << "  Failed: " << g_tests_failed << std::endl;           \
        return g_tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;            \
    }()

#endif // QE_TEST_FRAMEWORK_H
