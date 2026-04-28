// SPDX-License-Identifier: MIT
// Copyright (c) 2026 D-Sorganization. All rights reserved.

#include "../src/math/Quaternion.h"
#include "../src/math/Vec3.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>

namespace {

using Clock = std::chrono::steady_clock;
using qe::math::Quaternion;
using qe::math::Vec3;

constexpr float kPi = 3.14159265358979323846f;
constexpr int kIterations = 200000;

struct BenchmarkResult {
    const char* name;
    int iterations;
    double checksum;
    double elapsed_ms;
};

BenchmarkResult benchmark_quaternion_slerp_rotate() {
    const Quaternion start = Quaternion::identity();
    const Quaternion end =
        Quaternion::from_axis_angle(Vec3(0.25f, 1.0f, -0.5f), kPi * 0.875f);
    const Vec3 probe(0.7f, -0.2f, 0.4f);

    double checksum = 0.0;
    const auto begin = Clock::now();

    for (int i = 0; i < kIterations; ++i) {
        const float t = static_cast<float>(i % 1024) / 1023.0f;
        const Quaternion blended = Quaternion::slerp(start, end, t);
        const Vec3 rotated = blended.rotate(probe);
        checksum += static_cast<double>(rotated.x) * 0.25;
        checksum += static_cast<double>(rotated.y) * 0.50;
        checksum += static_cast<double>(rotated.z) * 0.75;
    }

    const auto elapsed =
        std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
    return {"quaternion_slerp_rotate", kIterations, checksum, elapsed};
}

BenchmarkResult benchmark_vec3_normalize_cross() {
    double checksum = 0.0;
    const auto begin = Clock::now();

    for (int i = 1; i <= kIterations; ++i) {
        const float scale = static_cast<float>((i % 97) + 1);
        const Vec3 a(scale * 0.25f, scale * -0.5f, scale * 0.75f);
        const Vec3 b(scale * -0.125f, scale * 0.375f, scale * 0.625f);
        const Vec3 normal = a.normalized();
        const Vec3 tangent = normal.cross(b.normalized());

        checksum += static_cast<double>(normal.dot(tangent));
        checksum += static_cast<double>(tangent.length_squared());
    }

    const auto elapsed =
        std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
    return {"vec3_normalize_cross", kIterations, checksum, elapsed};
}

bool validate_result(const BenchmarkResult& result) {
    if (result.iterations <= 0) {
        std::cerr << "Benchmark did not execute any iterations for " << result.name << '\n';
        return false;
    }

    if (!std::isfinite(result.checksum)) {
        std::cerr << "Benchmark checksum is not finite for " << result.name << '\n';
        return false;
    }

    if (std::abs(result.checksum) <= 1e-9) {
        std::cerr << "Benchmark checksum is unexpectedly zero for " << result.name << '\n';
        return false;
    }

    return true;
}

void print_result(const BenchmarkResult& result) {
    std::cout << "{"
              << "\"name\":\"" << result.name << "\","
              << "\"iterations\":" << result.iterations << ","
              << "\"elapsed_ms\":" << std::fixed << std::setprecision(3) << result.elapsed_ms
              << ","
              << "\"checksum\":" << std::setprecision(9) << result.checksum
              << "}" << '\n';
}

} // namespace

int main() {
    const BenchmarkResult quaternion = benchmark_quaternion_slerp_rotate();
    const BenchmarkResult vec3 = benchmark_vec3_normalize_cross();

    print_result(quaternion);
    print_result(vec3);

    bool ok = true;
    ok &= validate_result(quaternion);
    ok &= validate_result(vec3);

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
