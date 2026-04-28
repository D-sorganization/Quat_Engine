"""Hypothesis-backed properties for stable QuatEngine math behavior."""

from __future__ import annotations

import math
import os
import shutil
import subprocess
from pathlib import Path

import pytest
from hypothesis import HealthCheck, given, settings
from hypothesis import strategies as st

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = REPO_ROOT / "src"

FLOATS = st.floats(
    min_value=-10.0,
    max_value=10.0,
    allow_nan=False,
    allow_infinity=False,
    width=32,
)
ANGLES = st.floats(
    min_value=-math.pi,
    max_value=math.pi,
    allow_nan=False,
    allow_infinity=False,
    width=32,
)
UNIT_T = st.floats(
    min_value=0.0,
    max_value=1.0,
    allow_nan=False,
    allow_infinity=False,
    width=32,
)
NONZERO_VECTOR = (
    st.tuples(FLOATS, FLOATS, FLOATS).filter(
        lambda v: (v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) > 1e-4
    )
)


PROBE_SOURCE = r"""
#include "math/Quaternion.h"
#include "math/Vec3.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

using qe::math::Quaternion;
using qe::math::Vec3;

float arg(char** argv, int index) {
    return std::strtof(argv[index], nullptr);
}

void print_vec3(const Vec3& v) {
    std::cout << std::setprecision(9) << v.x << " " << v.y << " " << v.z;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        return 2;
    }

    std::string mode = argv[1];
    if (mode == "rotate_inverse" && argc == 9) {
        Vec3 axis(arg(argv, 2), arg(argv, 3), arg(argv, 4));
        Vec3 vector(arg(argv, 6), arg(argv, 7), arg(argv, 8));
        Quaternion q = Quaternion::from_axis_angle(axis, arg(argv, 5));
        Vec3 rotated = q.rotate(vector);
        Vec3 restored = q.inverse().rotate(rotated);
        std::cout << std::setprecision(9)
                  << q.norm() << " "
                  << vector.length() << " "
                  << rotated.length() << " ";
        print_vec3(restored);
        return 0;
    }

    if (mode == "align" && argc == 8) {
        Vec3 from(arg(argv, 2), arg(argv, 3), arg(argv, 4));
        Vec3 to(arg(argv, 5), arg(argv, 6), arg(argv, 7));
        Quaternion q = Quaternion::from_two_vectors(from, to);
        Vec3 rotated = q.rotate(from.normalized());
        Vec3 expected = to.normalized();
        std::cout << std::setprecision(9)
                  << q.norm() << " "
                  << rotated.dot(expected) << " "
                  << rotated.length();
        return 0;
    }

    if (mode == "slerp_norm" && argc == 11) {
        Vec3 axis_a(arg(argv, 2), arg(argv, 3), arg(argv, 4));
        Vec3 axis_b(arg(argv, 6), arg(argv, 7), arg(argv, 8));
        Quaternion a = Quaternion::from_axis_angle(axis_a, arg(argv, 5));
        Quaternion b = Quaternion::from_axis_angle(axis_b, arg(argv, 9));
        Quaternion result = Quaternion::slerp(a, b, arg(argv, 10));
        std::cout << std::setprecision(9) << result.norm();
        return 0;
    }

    return 2;
}
"""


def _compiler() -> str | None:
    configured = os.environ.get("CXX")
    if configured:
        return configured
    for candidate in ("c++", "g++", "clang++"):
        path = shutil.which(candidate)
        if path:
            return path
    return None


@pytest.fixture(scope="session")
def math_probe(tmp_path_factory: pytest.TempPathFactory) -> Path:
    compiler = _compiler()
    if compiler is None:
        pytest.skip("No C++ compiler found for Hypothesis math probe")

    build_dir = tmp_path_factory.mktemp("qe_math_probe")
    source = build_dir / "math_property_probe.cpp"
    executable = build_dir / ("math_property_probe.exe" if os.name == "nt" else "math_property_probe")
    source.write_text(PROBE_SOURCE, encoding="utf-8")

    subprocess.run(
        [
            compiler,
            "-std=c++17",
            "-I",
            str(SRC_DIR),
            str(source),
            "-o",
            str(executable),
        ],
        check=True,
        cwd=REPO_ROOT,
        text=True,
        capture_output=True,
    )
    return executable


def _run_probe(executable: Path, *args: float | str) -> list[float]:
    completed = subprocess.run(
        [str(executable), *(str(arg) for arg in args)],
        check=True,
        cwd=REPO_ROOT,
        text=True,
        capture_output=True,
    )
    return [float(part) for part in completed.stdout.split()]


@settings(max_examples=60, deadline=None, suppress_health_check=[HealthCheck.filter_too_much])
@given(axis=NONZERO_VECTOR, vector=NONZERO_VECTOR, angle=ANGLES)
def test_axis_angle_rotation_preserves_length_and_inverse_restores_vector(
    math_probe: Path,
    axis: tuple[float, float, float],
    vector: tuple[float, float, float],
    angle: float,
) -> None:
    q_norm, original_len, rotated_len, restored_x, restored_y, restored_z = _run_probe(
        math_probe,
        "rotate_inverse",
        *axis,
        angle,
        *vector,
    )

    assert q_norm == pytest.approx(1.0, abs=1e-4)
    assert rotated_len == pytest.approx(original_len, abs=1e-3)
    assert (restored_x, restored_y, restored_z) == pytest.approx(vector, abs=1e-3)


@settings(max_examples=60, deadline=None, suppress_health_check=[HealthCheck.filter_too_much])
@given(source=NONZERO_VECTOR, target=NONZERO_VECTOR)
def test_from_two_vectors_aligns_normalized_directions(
    math_probe: Path,
    source: tuple[float, float, float],
    target: tuple[float, float, float],
) -> None:
    q_norm, alignment, rotated_len = _run_probe(math_probe, "align", *source, *target)

    assert q_norm == pytest.approx(1.0, abs=1e-4)
    assert rotated_len == pytest.approx(1.0, abs=1e-4)
    assert alignment == pytest.approx(1.0, abs=1e-3)


@settings(max_examples=60, deadline=None, suppress_health_check=[HealthCheck.filter_too_much])
@given(axis_a=NONZERO_VECTOR, angle_a=ANGLES, axis_b=NONZERO_VECTOR, angle_b=ANGLES, t=UNIT_T)
def test_slerp_between_axis_angle_rotations_stays_unit_length(
    math_probe: Path,
    axis_a: tuple[float, float, float],
    angle_a: float,
    axis_b: tuple[float, float, float],
    angle_b: float,
    t: float,
) -> None:
    (result_norm,) = _run_probe(
        math_probe,
        "slerp_norm",
        *axis_a,
        angle_a,
        *axis_b,
        angle_b,
        t,
    )

    assert result_norm == pytest.approx(1.0, abs=1e-4)
